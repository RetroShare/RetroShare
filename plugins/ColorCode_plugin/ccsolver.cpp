/* ColorCode, a free MasterMind clone with built in solver
 * Copyright (C) 2009  Dirk Laebisch
 * http://www.laebisch.com/
 *
 * ColorCode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ColorCode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ColorCode. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ccsolver.h"
#include <time.h>

using namespace std;

const int CCSolver::STRENGTH_LOW    = 0;
const int CCSolver::STRENGTH_MEDIUM = 1;
const int CCSolver::STRENGTH_HIGH   = 2;

const int CCSolver::mFGCnts[4][9][4] = {
{
    {2, 0, 0, 0},
    {2, 0, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 0}
},
{
    {3, 0, 0, 0},
    {2, 1, 0, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {1, 1, 1, 0}
},
{
    {3, 1, 0, 0},
    {3, 1, 0, 0},
    {2, 1, 1, 0},
    {2, 2, 0, 0},
    {2, 2, 0, 0},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1}
},
{
    {4, 1, 0, 0},
    {3, 2, 0, 0},
    {3, 1, 1, 0},
    {2, 2, 1, 0},
    {2, 1, 1, 1},
    {2, 2, 1, 0},
    {2, 1, 1, 1},
    {2, 1, 1, 1},
    {2, 1, 1, 1}
}
};

CCSolver::CCSolver(ColorCode* cc)
{
    mCc = cc;

    mColorCnt    = 0;
    mPegCnt      = 0;
    mDoubles     = -1;
    mLevel       = -1;
    mMaxGuessCnt = 0;

    mAllCnt      = 0;
    mPosCnt      = 0;
    mHistCnt     = 0;
    mGuessCnt    = 0;
    mMaxRes      = 0;
    mResCnt      = 0;

    mRestartCnt  = 0;

    mResTable    = NULL;
    mCascade     = NULL;
    mPossible    = NULL;
    mAllTable    = NULL;
    mHist        = NULL;

    mS           = NULL;
    mP           = NULL;

    mBusy        = false;
}

CCSolver::~CCSolver()
{
    FreeTables();
}

void CCSolver::NewGame(const int ccnt, const int pcnt, const int doubles, const int level, const int gcnt)
{
    FreeTables();

    mColorCnt    = ccnt;
    mPegCnt      = pcnt;
    mDoubles     = doubles;
    mLevel       = level;
    mMaxGuessCnt = gcnt;

    mAllCnt      = 0;
    mPosCnt      = 0;
    mHistCnt     = 0;
    mGuessCnt    = 0;
    mMaxRes      = 0;
    mResCnt      = 0;

    mRestartCnt  = 0;
    mLastPosCnt  = 0;

    InitTables();
}

void CCSolver::RestartGame()
{
    if (mGuessCnt > 0)
    {
        ++mRestartCnt;
    }

    for (int i = 0 ; i < mMaxGuessCnt ; ++i)
    {
        mHist[i] = NULL;
    }

    mGuessCnt = 0;
}

void CCSolver::GuessIn(string str)
{
    int ix = Row2Ix(str);
    mHist[mGuessCnt] = new int[2];
    mHist[mGuessCnt][0] = ix;
}

void CCSolver::GuessIn(const vector<int>* rowsol)
{
    int ix = Row2Ix(rowsol);
    mHist[mGuessCnt] = new int[2];
    mHist[mGuessCnt][0] = ix;
}

int* CCSolver::GuessOut()
{
    if (mPosCnt == mLastPosCnt && (mGuessCnt > 0 || mRestartCnt > 0))
    {
        return mAllTable[mLastGuess];
    }

    int g = Guess();
    if (g < 0 || g >= mAllCnt)
    {
        return NULL;
    }

    mLastPosCnt = mPosCnt;
    mLastGuess = g;
    return mAllTable[g];
}

int CCSolver::ResIn(const vector<int>* res)
{
    int r = 0;
    int len = res->size();
    int i;
    for (i = 0; i < len; ++i)
    {
        if (res->at(i) == 2)
        {
            r += 10;
        }
        else if (res->at(i) == 1)
        {
            r += 1;
        }
    }

    mHist[mGuessCnt][1] = r;
    UpdPos(mHist[mGuessCnt][0], r);
    ++mGuessCnt;

    return r;
}

void CCSolver::FreeTables()
{
    int i;

    delete [] mCascade;
    mCascade = NULL;

    delete [] mPossible;
    mPossible = NULL;

    for (i = 0 ; i < mAllCnt ; ++i)
    {
        delete [] mAllTable[i] ;
    }
    delete [] mAllTable;
    mAllTable = NULL;

    for (i = 0 ; i < mMaxGuessCnt ; ++i)
    {
        delete [] mHist[i] ;
    }

    delete [] mHist;
    mHist = NULL;

    delete [] mResTable;
    mResTable = NULL;

    delete [] mS;
    mS = NULL;

    delete [] mP;
    mP = NULL;
}

void CCSolver::InitTables()
{
    int i, j, k;

    if (mDoubles != 1 && mColorCnt < mPegCnt)
    {
        return;
    }

    mCascade = new int [mPegCnt];

    for (i = 0; i < mPegCnt; ++i)
    {
        mCascade[i] = IntPow(mColorCnt, mPegCnt - i - 1);
    }

    mAllCnt = 1;
    for (i = 0; i < mPegCnt; ++i)
    {
        mAllCnt *= mColorCnt;
    }

    mAllTable = new int* [mAllCnt];

    for (i = 0; i < mAllCnt; ++i)
    {
        mAllTable[i] = new int [mPegCnt];
    }


    mPossible = new int [mAllCnt];

    for (i = 0; i < mAllCnt; ++i)
    {
        FillTable(i);
    }

    mHist = new int* [mMaxGuessCnt];
    for (i = 0 ; i < mMaxGuessCnt ; ++i)
    {
        mHist[i] = NULL;
    }

    mS = new int [mColorCnt];
    mP = new int [mColorCnt];

    mMaxRes = mPegCnt * 10;

    mResCnt = (IntFac(mPegCnt + 2)) / (IntFac(mPegCnt) * 2) - 1;

    mResTable = new int[mResCnt];

    k = 0;
    for (i = 0; i <= mPegCnt; ++i)
    {
        for (j = 0; j <= mPegCnt - i; ++j, ++k)
        {
            if (i == mPegCnt - 1 && j == 1)
            {
                --k;
                continue;
            }
            mResTable[k] = i * 10 + j;
        }
    }
}

void CCSolver::FillTable(const int ix)
{
    int i;
    int v = ix;
    int pos = 1;
    int j;

    for (i = 0; i < mPegCnt; ++i)
    {
        mAllTable[ix][i] = 0;
    }

    for (i = 0; i < mPegCnt; ++i)
    {
        while (v >= mCascade[i])
        {
            ++mAllTable[ix][i];
            v -= mCascade[i];
        }
        if (mDoubles != 1 && pos == 1)
        {
            for (j = 0; j < i; j++)
            {
                if (mAllTable[ix][j] == mAllTable[ix][i])
                {
                    pos = 0;
                    break;
                }
            }
        }
    }

    mPosCnt += pos;
    mPossible[ix] = pos;
}

int CCSolver::RowCmp(const int* sol, const int* pat)
{
    int bl = 0;
    int wh = 0;
    int i;

    for (i = 0; i < mColorCnt; ++i)
    {
        mS[i] = mP[i] = 0;
    }

    for (i = 0; i < mPegCnt; ++i)
    {
        if (sol[i] == pat[i])
        {
            ++bl;
        }
        else
        {
            if (mP[sol[i]] > 0)
            {
                ++wh;
                --mP[sol[i]];
            }
            else
            {
                ++mS[sol[i]];
            }

            if (mS[pat[i]] > 0)
            {
                ++wh;
                --mS[pat[i]];
            }
            else
            {
                ++mP[pat[i]];
            }
        }
    }

    return (10 * bl + wh);
}

void CCSolver::UpdPos(const int ix, const int res)
{
    int i;
    for (i = 0; i < mAllCnt; ++i)
    {
        if (!mPossible[i])
        {
            continue;
        }

        if (RowCmp(mAllTable[i], mAllTable[ix]) != res)
        {
            mPossible[i] = 0;
            --mPosCnt;
        }
    }
}

string CCSolver::FirstGuess()
{
    int i, j, r;
    int pegcnt = 0;
    int diffcnt = 0;

    const int* fg = mFGCnts[mPegCnt-2][mColorCnt-2];
    char s[mPegCnt];
    int t[mColorCnt];

    for (i = 0; i < mColorCnt; ++i)
    {
        t[i] = 0;
    }

    if (mDoubles == 1)
    {
        for (i = 0; i < 4; ++i)
        {
            if (fg[i] == 0)
            {
                break;
            }

            do
            {
                r = rand() % mColorCnt;
            }
            while (t[r] > 0);
            ++t[r];
            ++diffcnt;
            for (j = 0; j < fg[i]; ++j)
            {
                s[pegcnt++] = 'A' + r;
            }
            if (pegcnt == mPegCnt)
            {
                break;
            }
        }

        if (pegcnt != mPegCnt)
        {
            //
        }

        if (mPegCnt > 2 && diffcnt > 1)
        {
            for (int i = 0; i < (mPegCnt - 1); ++i)
            {
                r = i + (rand() % (mPegCnt - i));
                if (i != r)
                {
                    char tmp = s[i];
                    s[i] = s[r];
                    s[r] = tmp;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < mPegCnt; ++i)
        {
            do
            {
                r = rand() % mColorCnt;
            }
            while (t[r] > 0);
            ++t[r];
            s[i] = 'A' + r;
        }
    }

    return string(s, mPegCnt);
}

int CCSolver::Guess()
{
    clock_t start, finish;
    start = clock();

    if (mLevel > STRENGTH_LOW && mGuessCnt == 0 && mRestartCnt == 0)
    {
        return Row2Ix(FirstGuess());
    }

    mBusy = true;

    int level = mLevel;
    if (level == STRENGTH_HIGH && mPosCnt > 10000)
    {
        level = STRENGTH_MEDIUM;
    }

    int i, j;

    if (level == STRENGTH_LOW)
    {
        for (i = 0; i < mAllCnt; ++i)
        {
            if (mPossible[i])
            {
                break;
            }
        }
    }
    else if (level == STRENGTH_MEDIUM)
    {
        int m = mPosCnt >> 1;
        for (i = 0; i < mAllCnt; ++i)
        {
            if (mPossible[i])
            {
                if (--m <= 0)
                {
                    break;
                }
            }
        }
    }
    else
    {
        // Donald E. Knuth.  The Computer as Master Mind.  J. Recreational Mathematics, 9 (1976-77), 1-6.
        // Kenji Koyama, Tony W. Lai.  An optimal Mastermind Strategy.  J. Recreational Mathematics, 1994.
        // we'll only take the still possible ones into account, but ...

        int solix = -1;
        int shortest = 1000000;
        int longest;
        int res2pos[mMaxRes];

        for (j = 0; j <= mMaxRes; ++j)
        {
            res2pos[j] = 0;
        }

        for (i = 0; i < mAllCnt; ++i)
        {
            if (!mPossible[i])
            {
                continue;
            }

            for (j = 0; j < mAllCnt; ++j)
            {
                if (!mPossible[j])
                {
                    continue;
                }
                ++res2pos[RowCmp(mAllTable[j], mAllTable[i])];
            }

            longest = 0;
            for (j = 0; j < mResCnt; ++j)
            {
                if (res2pos[mResTable[j]] > longest)
                {
                    longest = res2pos[mResTable[j]];
                }
                res2pos[mResTable[j]] = 0;
            }

            if (longest < shortest)
            {
                shortest = longest;
                solix = i;
            }
        }

        i = solix;
    }

    finish = clock();
    mBusy = false;

    return i;
}

string CCSolver::RowOut(const int* r)
{
    char s[mPegCnt];

    int i;
    for (i = 0; i < mPegCnt; ++i)
    {
        s[i] = 'A' + r[i];
    }

    return string(s, mPegCnt);
}

int CCSolver::Row2Ix(const vector<int>* v)
{
    int i;
    int ix = 0;
    int len = v->size();

    if (len != mPegCnt)
    {
        return -1;
    }

    int j;
    for (i = 0; i < mPegCnt; ++i)
    {
        j = v->at(i);
        if (j >= mColorCnt)
        {
            ix = -1;
            break;
        }
        ix += j * mCascade[i];
    }

    if (ix >= 0 && ix < mAllCnt)
    {
        return ix;
    }
    return -1;
}

int CCSolver::Row2Ix(const string str)
{
    int i;
    int ix = 0;
    int len = int(str.size());

    if (len != mPegCnt)
    {
        return -1;
    }

    int j;
    for (i = 0; i < mPegCnt; ++i)
    {
        j = toupper(int(str[i])) - 'A';
        if (j >= mColorCnt)
        {
            ix = -1;
            break;
        }
        ix += j * mCascade[i];
    }

    if (ix >= 0 && ix < mAllCnt)
    {
        return ix;
    }
    return -1;
}

int CCSolver::IntPow(const int b, const int e)
{
    int i;
    int ret = 1;
    for (i = 0; i < e; ++i)
    {
        ret *= b;
    }
    return ret;
}

int CCSolver::IntFac(const int n)
{
    int fac = 1;
    int i;
    for (i = n; i > 1; --i)
    {
        fac *= i;
    }
    return fac;
}
