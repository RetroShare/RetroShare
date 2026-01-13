# Copilot Prompt: Implement Dynamic Default Logo Generation in RetroShare

## Objective
Implement a "Logo Creator" system that generates deterministic, colored default logos for Friend Nodes, Channels, Boards, and Wikis when no custom avatar or logo is set. These logos should be cached locally using the existing `GxsIdDetails` caching mechanism.

## Context
RetroShare currently uses static placeholder icons (e.g., `:icons/png/channel.png`) when a node or group lacks an image. The goal is to replace these with dynamically generated icons consisting of:
1.  **Background**: A colored circle where the color is derived from a hash of the `sslId` (for nodes) or `groupId` (for GXS groups).
2.  **Foreground**: A category-specific icon (Buddy, Channel, Board, Wiki) overlaid on the background.

## Technical Requirements

### 1. Create `LogoGenerator` Utility
Create a new utility (or add to `GxsIdDetails`) with a static method:
`static QPixmap generate(const QString &id, const QString &iconPath, int size)`

- **Color Logic**: Use `qHash(id)` to get a stable integer. Use this hash to set the `Hue` in HSL color space. Maintain `Saturation` around 150 and `Lightness` around 180 for a consistent "pastel" look.
- **Drawing**:
    - Draw an anti-aliased ellipse with the generated color.
    - Scale the foreground icon (from `iconPath`) to ~70% of the total size.
    - Center the icon on the circle.

### 2. Update `GxsIdDetails` (Caching)
Extend `GxsIdDetails` to support these logos:
- Add a method `static QPixmap makeDefaultGroupIcon(const RsGxsId& id, const QString& iconPath, AvatarSize size)`.
- Use the existing `mDefaultIconCache` to store these generated pixmaps.
- Ensure the cache cleaning logic (`checkCleanImagesCache`) handles these entries correctly.

### 3. Integrate into UI Dialogs

#### Friend Nodes (`gui/common/AvatarDefs.cpp`)
- In `AvatarDefs::getAvatarFromSslId`, if `size == 0`, instead of returning a static default image, call the new `LogoGenerator` or a helper in `GxsIdDetails` using the `sslId`.
- **New Task**: In `AvatarDefs::getAvatarFromGpgId`, if `size == 0`, use the dynamic logo generator. Pass the `gpgId` (converted to string or handled appropriately) to `makeDefaultGroupIcon` or similar, using a buddy/person icon.

#### Channels (`gui/gxschannels/GxsChannelDialog.cpp`)
- In `GxsChannelDialog::groupInfoToGroupItemInfo`, replace:
  `groupItemInfo.icon = FilesDefs::getIconFromQtResourcePath(":icons/png/channel.png");`
  with:
  ```cpp
  RsGxsGroupId grpId = channelGroupData->mGroupId;
  groupItemInfo.icon = GxsIdDetails::makeDefaultGroupIcon(RsGxsId(grpId), ":icons/png/channel.png", GxsIdDetails::ORIGINAL);
  ```

#### Boards (`gui/Posted/PostedDialog.cpp`)
- In `PostedDialog::groupInfoToGroupItemInfo`, replace the static `:icons/png/postedlinks.png` with:
  ```cpp
  RsGxsGroupId grpId = postedGroupData->mGroupId;
  groupItemInfo.icon = GxsIdDetails::makeDefaultGroupIcon(RsGxsId(grpId), ":icons/png/postedlinks.png", GxsIdDetails::ORIGINAL);
  ```

#### Wiki (`gui/WikiPoos/WikiDialog.cpp`)
- In `WikiDialog::GroupMetaDataToGroupItemInfo`, replace:
  `groupItemInfo.icon = QIcon(IMAGE_WIKI);`
  with the dynamic generator using `groupInfo.mGroupId`.

## File References
- `retroshare-gui/src/gui/gxs/GxsIdDetails.cpp` (Base for caching and generation logic)
- `retroshare-gui/src/gui/common/AvatarDefs.cpp` (Friend node avatars)
- `retroshare-gui/src/gui/gxschannels/GxsChannelDialog.cpp` (Channels)
- `retroshare-gui/src/gui/Posted/PostedDialog.cpp` (Boards)
- `retroshare-gui/src/gui/WikiPoos/WikiDialog.cpp` (Wiki)

## Implementation Notes
- Follow the existing Qt coding style (naming conventions, includes).
- Use `QPainter` for the image composition.
- Ensure thread safety by using existing mutexes in `GxsIdDetails` when accessing the cache.
