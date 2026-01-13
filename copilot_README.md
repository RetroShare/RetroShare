# RetroShare: Dynamic Default Logo Generation

This project implements a deterministic "Logo Creator" for RetroShare. It replaces static placeholder icons with generated, colored backgrounds and category-specific overlays for Friend Nodes, Channels, Boards, and Wikis.

## Feature Overview
- **Deterministic Colors**: Background colors are generated based on a hash of the node/group ID.
- **Category Icons**: Unique foreground icons for Buddies (Friend Nodes), Channels, Boards, and Wikis.
- **Local Caching**: Generated logos are cached using the existing `GxsIdDetails` infrastructure to ensure high performance and low memory overhead.

## Implementation Details

### 1. Logo Generation Logic
The logic resides in `GxsIdDetails`. It uses `QPainter` to:
1.  Calculate a stable hue from the ID hash.
2.  Draw a colored circle (HSL: Hue from ID, Saturation 150, Lightness 180).
3.  Overlay a category-specific PNG (e.g., `:icons/png/channel.png`) scaled to 70% of the background.

### 2. UI Integration Points

#### Friend Nodes & Identities
- `gui/common/AvatarDefs.cpp`:
    - `getAvatarFromSslId`: Generates default logo if no avatar data is found.
    - `getAvatarFromGpgId`: Ensures the top-level Friend items use dynamic logos when missing an avatar.

#### GXS Groups
- `gui/gxschannels/GxsChannelDialog.cpp`:
    - `groupInfoToGroupItemInfo`: Resolves `grpId` and calls `makeDefaultGroupIcon`.
- `gui/Posted/PostedDialog.cpp`:
    - `groupInfoToGroupItemInfo`: Resolves `grpId` and calls `makeDefaultGroupIcon`.
- `gui/WikiPoos/WikiDialog.cpp`:
    - `GroupMetaDataToGroupItemInfo`: Uses dynamic generation for Wiki group icons.

## How to use with Copilot
If you are using Copilot to continue implementation or debugging, refer to `copilot_prompt.md` for specific technical instructions and code snippets.
