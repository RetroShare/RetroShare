# RetroShare

## Overview

RetroShare is a decentralized, private, secure, cross-platform communication toolkit. It provides file sharing, chat, messages, forums, channels, boards, wikis, and more. The application is built primarily in C++ using the Qt framework (supporting both Qt5 and Qt6) and follows a modular architecture with a core library (libretroshare), GUI client (retroshare-gui), headless service (retroshare-service), and optional plugins (VOIP, FeedReader).

## User Preferences

Preferred communication style: Simple, everyday language.

## System Architecture

### Core Components

**libretroshare** - The core library containing all backend functionality including:
- Peer-to-peer networking and DHT (distributed hash table)
- Cryptographic operations via OpenPGP SDK and Botan
- GXS (General eXchange System) for decentralized data distribution
- Services for chat, forums, channels, file sharing, etc.

**retroshare-gui** - Qt-based desktop GUI application that provides the user interface. Uses the MVC pattern with:
- Custom widgets and dialogs for each feature (chat, forums, channels, etc.)
- Avatar and identity management via GxsIdDetails with image caching
- Resource files and internationalization support (20+ languages)

**retroshare-service** - Headless service exposing libretroshare via JSON API over HTTP (using restbed library). Designed for server deployments and integration with other applications.

### Plugin System

Plugins extend functionality without modifying core code:
- **VOIP** - Voice/video calls using Speex, OpenCV, and Qt Multimedia
- **FeedReader** - RSS/Atom feed aggregation using libxml2, libxslt, and libcurl

### Build System

- **CMake** is the primary build system
- **qmake** (.pro files) still supported for some components
- Cross-platform support: Linux, Windows (MSYS2/MinGW), macOS
- Submodules: libbitdht, libretroshare, openpgpsdk are managed as git submodules

### Data Storage

- **SQLCipher** - Encrypted SQLite database for local data persistence
- Local caching for avatars, images, and identity information (see GxsIdDetails)
- File-based configuration and key storage

### Security Architecture

- End-to-end encryption using OpenPGP for identity and message encryption
- Friend-to-friend network model with certificate-based authentication
- SSL/TLS for transport security (OpenSSL)
- Optional Tor integration for anonymity

## External Dependencies

### Required Libraries
- **Qt5 or Qt6** - GUI framework (QtBase, QtMultimedia, QtX11Extras/Qt5Compat)
- **OpenSSL** - Cryptographic operations and secure transport
- **SQLCipher** - Encrypted database storage
- **Botan** - Additional cryptographic primitives
- **libupnp** - UPnP for NAT traversal
- **bzip2** - Compression
- **json-c / rapidjson** - JSON parsing

### Optional Dependencies
- **restbed** - HTTP server for JSON API (retroshare-service)
- **libsecret** - Secure credential storage for autologin (Linux)
- **libxml2/libxslt/libcurl** - FeedReader plugin
- **Speex/OpenCV/Qt Multimedia** - VOIP plugin

### Build Tools
- CMake 3.18+
- C++ compiler with C++17 support (GCC, Clang, MSVC)
- Qt build tools (qmake, moc, uic, rcc)
- Doxygen for documentation