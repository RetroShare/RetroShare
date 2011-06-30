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

#ifndef CCSOLVER_H
#define CCSOLVER_H

#include <iostream>
#include "colorcode.h"

using namespace std;

class CCSolver
{
public:
    static const int STRENGTH_LOW;
    static const int STRENGTH_MEDIUM;
    static const int STRENGTH_HIGH;

    CCSolver(ColorCode* cc);
    ~CCSolver();

    void NewGame(const int ccnt, const int pcnt = 4, const int doubles = 1, const int level = 2, const int gcnt = 10);
    void RestartGame();
    string FirstGuess();
    void GuessIn(string str);
    void GuessIn(const vector<int>* rowsol);
    int ResIn(const vector<int>* res);
    int* GuessOut();

    volatile bool mBusy;

private:
    static const int mFGCnts[4][9][4];

    ColorCode* mCc;

    int mMaxGuessCnt;
    int mColorCnt;
    int mPegCnt;
    int mDoubles;
    int mLevel;

    int mLastGuess;
    int mLastPosCnt;

    int mAllCnt;
    int mPosCnt;
    int mHistCnt;
    int mGuessCnt;
    int mMaxRes;
    int mResCnt;

    int mRestartCnt;

    int* mResTable;
    int* mCascade;
    int* mPossible;
    int** mAllTable;
    int** mHist;

    int* mS;
    int* mP;

    void FreeTables();
    void InitTables();
    void FillTable(const int ix);
    int RowCmp(const int* sol, const int* pat);
    void UpdPos(const int ix, const int res);
    int Guess();

    string RowOut(const int* r);
    int Row2Ix(const string str);
    int Row2Ix(const vector<int>* v);

    int IntPow(const int b, const int e);
    int IntFac(const int n);
};

#endif // CCSOLVER_H
