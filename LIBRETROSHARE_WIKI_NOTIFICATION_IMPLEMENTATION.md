# LibRetroShare Wiki Notification Implementation

## Objective
Add synchronous helper methods to `RsGxsIfaceHelper` class in libretroshare to support GUI notification counting for the Wiki service. These methods will enable `WikiUserNotify` in retroshare-gui to efficiently count unread messages without complex async token management.

## Background
The RetroShare GUI uses a notification system (`UserNotify` subclasses) to display notification counts for various services (Forums, Channels, Posted, etc.). The Wiki service (`RsWiki`) needs similar notification support.

Currently, `RsGxsIfaceHelper` provides only an asynchronous token-based API which requires:
1. Calling `requestServiceStatistic()` to get a token
2. Calling `waitToken()` to block until data is ready  
3. Calling `getServiceStatistic()` with the token to retrieve results

This is cumbersome for simple notification counting in GUI code.

## Current State Analysis

### Existing Files in libretroshare
- **`src/retroshare/rsgxsifacehelper.h`** - Base helper class for GXS services, provides token-based async API
- **`src/retroshare/rswiki.h`** - Public Wiki service interface (inherits from `RsGxsIfaceHelper`)
- **`src/services/p3wiki.h/cc`** - Wiki service implementation (inherits from `RsGenExchange` and `RsWiki`)
- **`src/retroshare/rsgxsifacetypes.h`** - Contains `GxsServiceStatistic` structure with notification counts

### GxsServiceStatistic Structure
```cpp
struct GxsServiceStatistic {
    uint32_t mNumMsgs;
    uint32_t mNumGrps;
    uint32_t mSizeOfMsgs;
    uint32_t mSizeOfGrps;
    uint32_t mNumGrpsSubscribed;
    uint32_t mNumThreadMsgsNew;      // New root messages (threads)
    uint32_t mNumThreadMsgsUnread;   // Unread root messages
    uint32_t mNumChildMsgsNew;       // New child messages (comments)
    uint32_t mNumChildMsgsUnread;    // Unread child messages
    uint32_t mSizeStore;
};
```

### How Other Services Work
Looking at `GxsUserNotify` in retroshare-gui (used for Forums, Channels, etc.):
```cpp
void GxsUserNotify::startUpdate()
{
    mNewThreadMessageCount = 0;
    mNewChildMessageCount = 0;

    GxsServiceStatistic stats;
    mGroupFrameDialog->getServiceStatistics(stats);

    mNewThreadMessageCount = stats.mNumThreadMsgsNew;
    mNewChildMessageCount = stats.mNumChildMsgsNew;

    update();
}
```

The `GxsGroupFrameDialog` provides `getServiceStatistics()` which internally uses the async API and blocks. However, `WikiUserNotify` doesn't have access to a `GxsGroupFrameDialog` since the Wiki GUI is incomplete.

## Required Implementation

### Add Synchronous Helper Methods to RsGxsIfaceHelper

Add the following **protected** methods to `RsGxsIfaceHelper` class in `src/retroshare/rsgxsifacehelper.h`:

```cpp
protected:
    /**
     * @brief Get service statistics synchronously (blocking call)
     * @param stats Output parameter for service statistics
     * @param maxWait Maximum time to wait for the operation in milliseconds (default: 5000ms)
     * @return true if statistics were successfully retrieved, false on timeout or error
     * 
     * This is a convenience wrapper around the async token-based API.
     * It blocks the calling thread until results are available or timeout occurs.
     * Use this for simple operations like notification counting in GUI code.
     */
    bool getServiceStatisticsBlocking(
        GxsServiceStatistic& stats,
        std::chrono::milliseconds maxWait = std::chrono::milliseconds(5000)
    )
    {
        uint32_t token;
        if (!requestServiceStatistic(token))
            return false;
        
        auto status = waitToken(token, maxWait);
        if (status != RsTokenService::COMPLETE)
            return false;
        
        return getServiceStatistic(token, stats);
    }

    /**
     * @brief Get group statistics synchronously (blocking call)
     * @param grpId The group ID to get statistics for
     * @param stats Output parameter for group statistics
     * @param maxWait Maximum time to wait for the operation in milliseconds (default: 5000ms)
     * @return true if statistics were successfully retrieved, false on timeout or error
     * 
     * This is a convenience wrapper around the async token-based API.
     * It blocks the calling thread until results are available or timeout occurs.
     */
    bool getGroupStatisticBlocking(
        const RsGxsGroupId& grpId,
        GxsGroupStatistic& stats,
        std::chrono::milliseconds maxWait = std::chrono::milliseconds(5000)
    )
    {
        uint32_t token;
        if (!requestGroupStatistic(token, grpId))
            return false;
        
        auto status = waitToken(token, maxWait);
        if (status != RsTokenService::COMPLETE)
            return false;
        
        return getGroupStatistic(token, stats);
    }
```

### Why Protected, Not Public
These methods should be **protected** (not public) because:
1. They're designed to be used by derived service classes (like `RsWiki`)
2. Service-specific interfaces can expose them if needed via their own public methods
3. This follows the existing pattern where `waitToken()` is also protected
4. It prevents direct misuse by GUI code that should use service-specific interfaces

### Add Public Methods to RsWiki Interface

In `src/retroshare/rswiki.h`, add public methods that expose the statistics functionality:

```cpp
public:
    /**
     * @brief Get Wiki service statistics for notification counting
     * @param stats Output parameter for service statistics including unread message counts
     * @return true if statistics were successfully retrieved, false otherwise
     * 
     * This method is designed for GUI notification systems to efficiently count
     * new/unread messages across all Wiki collections.
     */
    virtual bool getWikiStatistics(GxsServiceStatistic& stats) = 0;
```

### Implement in p3Wiki Service

In `src/services/p3wiki.h`, add the declaration:

```cpp
public:
    virtual bool getWikiStatistics(GxsServiceStatistic& stats) override;
```

In `src/services/p3wiki.cc`, add the implementation:

```cpp
bool p3Wiki::getWikiStatistics(GxsServiceStatistic& stats)
{
    // Use the protected blocking helper from RsGxsIfaceHelper
    return getServiceStatisticsBlocking(stats);
}
```

## Testing Considerations

1. **Thread Safety**: The blocking methods use `waitToken()` which is already thread-safe with proper mutex handling
2. **Timeout Handling**: Default 5-second timeout should be sufficient for local operations
3. **Error Handling**: Methods return false on failure, allowing callers to handle errors gracefully
4. **Performance**: Blocking is acceptable for notification updates which happen every few seconds

## Usage Example (for reference)

After implementation, the GUI code in `WikiUserNotify::startUpdate()` will use:

```cpp
void WikiUserNotify::startUpdate()
{
    mNewCount = 0;
    
    if (mInterface)
    {
        GxsServiceStatistic stats;
        // mInterface is RsWiki*, which is-a RsGxsIfaceHelper
        if (static_cast<RsWiki*>(mInterface)->getWikiStatistics(stats))
        {
            // Count new messages (both threads and child messages)
            mNewCount = stats.mNumThreadMsgsNew + stats.mNumChildMsgsNew;
        }
    }
    
    update();
}
```

## Files to Modify

1. **`libretroshare/src/retroshare/rsgxsifacehelper.h`**
   - Add `getServiceStatisticsBlocking()` method (protected)
   - Add `getGroupStatisticBlocking()` method (protected)

2. **`libretroshare/src/retroshare/rswiki.h`**
   - Add `getWikiStatistics()` pure virtual method (public)

3. **`libretroshare/src/services/p3wiki.h`**
   - Add `getWikiStatistics()` override declaration

4. **`libretroshare/src/services/p3wiki.cc`**
   - Add `getWikiStatistics()` implementation

## Notes

- These methods are **blocking** by design - they're meant for GUI operations where blocking a few milliseconds is acceptable
- The async token-based API remains available for operations requiring fine-grained control
- This pattern can be reused for other GXS services if needed
- The implementation should include proper error logging using `RsErr()` or `RsWarn()` for debugging

## Compilation Requirements

- No new dependencies required
- Uses existing `<chrono>` header already included in rsgxsifacehelper.h
- Compatible with C++14 standard used by RetroShare
- Should compile on all supported platforms (Linux, Windows, macOS, Android)

## Alternative Considered (Not Recommended)

An alternative would be to expose these methods directly in `RsGxsIfaceHelper` as public methods. However, this is not recommended because:
- It would expose low-level blocking operations that might be misused
- Service-specific interfaces provide better API boundaries
- Protected methods maintain encapsulation while allowing inheritance-based access

## Summary

This implementation provides a clean, type-safe way for GUI notification systems to access unread message counts without dealing with the complexity of async token management. The approach:
- ✅ Maintains backward compatibility (no existing API changes)
- ✅ Follows existing RetroShare patterns
- ✅ Provides proper error handling
- ✅ Is thread-safe
- ✅ Has reasonable performance characteristics for GUI updates
