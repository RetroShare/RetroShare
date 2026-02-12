# Wiki Features Implementation Plan

## Overview
Implementing async loading, merge, republish & comments features for RetroShare Wiki.
Bounty: $30 from GitPay

## Stage 1: Replace RsGxsUpdateBroadcastPage with async loading
**Goal**: Modernize WikiDialog to use RsThread::async instead of TokenQueue
**Success Criteria**: 
- WikiDialog no longer inherits from RsGxsUpdateBroadcastPage
- All TokenQueue usage removed
- Data loading uses RsThread::async pattern
**Tests**: Compile successfully, Wiki groups and pages load correctly
**Status**: In Progress

### Changes needed:
1. WikiDialog.h:
   - Remove inheritance from `RsGxsUpdateBroadcastPage` and `TokenResponse`
   - Remove `TokenQueue *mWikiQueue`
   - Remove `loadRequest()` method
   - Remove `updateDisplay()` method
   - Add async loading methods

2. WikiDialog.cpp:
   - Replace all `mWikiQueue->requestGroupInfo()` with `RsThread::async()`
   - Replace all `mWikiQueue->requestMsgInfo()` with `RsThread::asy# Wiki Features Implementation Plan

## Overview
Imp- 
## Overview
Implementing async lo   Implementi`rBounty: $30 from GitPay

## Stage 1: Replace RsGxsUpdateBroadcastPage with async loans
## Stage 1: Replace Rize**Goal**: Modernize WikiDialog to use RsThread::async instead : **Success Criteria**: 
- WikiDialog no longer inherits from RsGxsUpdateBroen- WikiDialog no longe**- All TokenQueue usage removed
- Data loading uses RsThread:Ev- Data loading uses RsThread:ha**Tests**: Compile successfully, Wiki groune**Status**: In Progress

### Changes needed:
1. WikiDialog.h:
   - Rev
### Changes needed:
1ntC1. WikiDialog.h:
 r    - Remove inhpr   - Remove `TokenQueue *mWikiQueue`
   - Remove `loadRequest()` method
  l*   - Remove `loadRequest()` method
nd   - Remove `updateDisplay()` metri   - Add async loading methods

2. rg
2. WikiDialog.cpp:
   - Replrep   - Replace all es   - Replace all `mWikiQueue->requestMsgInfo()` with `RsThread::asy# Wikiub
## Overview
Imp- 
## Overview
Implementing async lo   Implementi`rBounty: $30 from GitPay

## Stage ageImp- 
## Oer## OfyImplementisu
## Stage 1: Replace RsGxsUpdateBroadcastPage with async low/## Stage 1: Replace Rize**Goal**: Modernize WikiDialog to usvi- WikiDialog no longer inherits from RsGxsUpdateBroen- WikiDialog no longe**- All TokenQueue usage removed
- ed- Data loading uses RsThread:Ev- Data loading uses RsThread:ha**Tests**: Compile successfully, Wiki grounr 
### Changes needed:
1. WikiDialog.h:
   - Rev
### Changes needed:
1ntC1. WikiDialog.h:
 r    - Remove inhpr   - Remove `TokenQu/fi1. WikiDialog.h:
 Te   - Rev
### Chse### Chaag1ntC1. WikiDialog.y
 r    - Remove inhprt   - Remove `loadRequest()` method
  l*   - Remove `loaAd  l*   - Remove `loadRequest()` ms
nd   - Remove `updateDisplay()` metrid 
2. rg
2. WikiDialog.cpp:
   - Replrep   - Replace all es   - Repl No2. War   - Replrep   - Im## Overview
Imp- 
## Overview
Implementing async lo   Implementi`rBounty: $30 from GitPay

## Stage ageImodImp- 
## Oed## O wImplementies
## Stage ageImp- 
## Oer## OfyImplementisu
## Stage 1: Re St## Oer## OfyImpl a## Stage 1: Replace RsGmi- ed- Data loading uses RsThread:Ev- Data loading uses RsThread:ha**Tests**: Compile successfully, Wiki grounr 
### Changes needed:
1. WikiDialog.h:
   - Rev
### Changes needed:
1ntC1. WikiDia:
cp retroshare-gui/src/gui/WikiPoos/WikiDialog.h retroshare-gui/src/gui/WikiPoos/WikiDialog.h.backup
:
cp retroshare-gui/src/gui/WikiPoos/WikiDialog.cpp retroshare-gui/src/gui/WikiPoos/WikiDialog.cpp.backup
:
cat retroshare-gui/src/gui/WikiPoos/WikiDialog.h
:
cat retroshare-gui/src/gui/WikiPoos/WikiDialog.cpp
:
head -200 retroshare-gui/src/gui/WikiPoos/WikiDialog.h

:
pwd

:
echo "Creating analysis document..."
