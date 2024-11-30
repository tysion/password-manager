# Password Manager Service

A password management service with a Telegram bot interface. This project integrates securely with PostgreSQL for data storage, supports encryption, JWT-based authentication, and TOTP-based 2FA, while offering a user-friendly bot for seamless interaction.

---

## Features
- **Secure Password Storage**: Store, retrieve, and manage your passwords securely.
- **Telegram Bot Interface**: Interact with the service via a bot, including password generation and management.
- **Robust Authentication**: Master key and TOTP-based authentication with session tokens stored in Redis.
- **PostgreSQL Database**: Reliable and scalable storage for user data.
- **Docker Support**: Easy deployment and management via Docker and Docker Compose.

---

## Build Instructions

### Prerequisites
- **C++ Compiler**: Clang 15+.
- **CMake**: Version 3.12 or later.
- **PostgreSQL**: Version 15.
- **Redis**: Version 6+ for session management.
- **Docker & Docker Compose**: For containerized setup.
- **Python**: Version 3.10+ for integration tests.

### Build the Service
Clone the repository:
 ```bash
 git clone https://github.com/your-repo/vaulty.git
 cd vaulty
 ```

Build the service using CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(nproc)
```

Run the service:
```
./vaulty --config ../configs/static_config.yaml --config_vars ../configs/config_vars.yaml
```

---

## Docker Setup
Build Docker images for the service:
```
docker-compose build
```

Start the service along with PostgreSQL:
```
docker-compose up -d
```

Check logs for the service:
```
docker logs vaulty_service
```

---

## Testing Instructions

### Unit Tests
Build and run unit tests:
```
cmake --build . --target vaulty_unittest
./vaulty_unittest
```

### Integration Tests
Configure .env for integration tests:
```bash
POSTGRES_PASSWORD=<RANDOM PASSWORD>
REDIS_PASSWORD=<RANDOM PASSWORD>
JWT_SECRET_KEY=<RANDOM PASSWORD>
CRYPTO_AES_256_BASE64_KEY=<RANDOM AES 256 BASE64 KEY>
TELEGRAM_BOT_TOKEN=<GET YOUR TOKEN FRON BotFather>
```

Run tests with pytest:
```bash
cd tests
pytest test_service.py
```

## Telegram Bot (only in Russian yet)

### Features
- **Password Management**: Add, retrieve, and delete passwords directly via the bot.
- **TOTP Integration**: Simplified onboarding with guidance for Google Authenticator setup.
- **Password Generator**: Quickly generate secure passwords with /gen.

### Usage

Start a chat with the bot and use /help for guidance on available commands.

https://t.me/TheVaultyBot

---

## Future Work

Full-text search:
- Add full-text search functionality for the service field in stored passwords to improve usability.

Telegram Bot Improvements:
- Add advanced search capabilities, better error handling, and session persistence enhancements.

**Contributions are welcome! Submit issues, feature requests, or pull requests to help improve the service.**
