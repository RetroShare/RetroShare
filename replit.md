# RetroShare

## Overview

RetroShare is a decentralized, private, secure, cross-platform communication toolkit. It provides file sharing, chat, messages, forums, channels, boards, wikis, and more. The project is built primarily in C++ using the Qt framework (supporting both Qt5 and Qt6) and targets multiple platforms including Windows, macOS, and Linux.

The codebase is organized as a modular system with:
- **libretroshare**: Core library handling networking, cryptography, and decentralized protocols
- **retroshare-gui**: Qt-based graphical user interface
- **retroshare-service**: Headless service with optional JSON API
- **plugins**: Extensible plugin system (VOIP, FeedReader, etc.)

## User Preferences

Preferred communication style: Simple, everyday language.

## System Architecture

### Build System
- **CMake**: Primary build system for the project
- **qmake**: Alternative build approach via Qt Creator
- Cross-platform compilation supported through platform-specific build scripts in `build_scripts/`

### Core Components

**libretroshare (submodule)**
- Handles all peer-to-peer networking and cryptographic operations
- Manages decentralized identity system (GXS - General eXchange System)
- Provides services for chat, forums, channels, file sharing

**retroshare-gui**
- Qt-based desktop application
- Modular page-based UI architecture (`MainPage`, `HomePage`, `ChatLobbyWidget`, etc.)
- Custom widget system with caching for performance (e.g., `GxsIdDetails` for identity avatars)
- Resource-based theming with icons and stylesheets

**retroshare-service**
- Headless operation mode
- JSON API via restbed for programmatic access
- Terminal-based login support

### Plugin Architecture
- Plugins extend functionality without modifying core
- Current plugins: VOIP (voice/video), FeedReader (RSS)
- Plugins integrate via notification system and settings windows

### Key Design Patterns
- **Caching System**: Images and avatars cached with automatic cleanup (`GxsIdDetails::checkCleanImagesCache`)
- **Smart Pointers**: QImage's built-in reference counting supplemented by application-level cache
- **Submodule Architecture**: Core libraries maintained as git submodules for modularity

### Current Feature Request Context
The codebase includes documentation for implementing dynamic default logo generation:
- Generate deterministic colored backgrounds from hash of IDs
- Overlay category-specific icons (Buddy, Channel, Board, Wiki)
- Integrate with existing `GxsIdDetails` caching infrastructure
- Touch points: `AvatarDefs.cpp`, `GxsChannelDialog.cpp`, `PostedDialog.cpp`, `WikiDialog.cpp`

## External Dependencies

### Required Libraries
- **Qt 5.15+ or Qt 6**: UI framework
- **OpenSSL**: Cryptographic operations
- **SQLCipher**: Encrypted database storage
- **libupnp**: NAT traversal
- **Botan 2**: Additional cryptography
- **RapidJSON/json-c**: JSON parsing
- **bzip2**: Compression

### Optional Dependencies
- **libxml2/libxslt/libcurl**: FeedReader plugin
- **libavcodec/speexdsp**: VOIP plugin
- **libsecret**: Autologin feature (Linux)

### Git Submodules
- `libbitdht/`: DHT implementation
- `libretroshare/`: Core library
- `openpgpsdk/`: OpenPGP implementation
- `retroshare-webui/`: Web interface (optional)
- `supportlibs/`: restbed, rapidjson, librnp

### Platform-Specific
- **Windows**: MSYS2/MinGW toolchain
- **macOS**: Xcode command line tools
- **Linux**: Distribution package managers (apt, dnf, zypper)