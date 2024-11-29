# Password Manager Service

A password management service with a Telegram bot interface. This project provides APIs for managing passwords and integrates with PostgreSQL for data storage. It ensures security with encryption, JWT-based authentication, and future support for TOTP.

---

## Features
- **Password storage**: Save, retrieve, and manage passwords securely.
- **Telegram bot interface**: Interact with the service through a user-friendly bot.
- **Secure authentication**: Utilizes master keys and JWT for user authentication.
- **PostgreSQL database**: Robust storage for users and passwords.

---

## Build Instructions

### Prerequisites
- **C++ Compiler**: Clang 15+.
- **CMake**: Version 3.12 or higher.
- **PostgreSQL**: Version 15.
- **Docker**: Optional, for containerized setup.
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
POSTGRES_USER=vaulty
POSTGRES_PASSWORD=password
POSTGRES_DB=vaulty
BASE_URL=http://localhost:8080/api/v1
```

Run tests with pytest:
```bash
cd tests
pytest test_service.py
```

---

## Future Work

TOTP support:
- Implement time-based one-time password (TOTP) authentication for enhanced security.

Full-text search:
- Add full-text search functionality for the service field in stored passwords to improve usability.

Telegram Bot Enhancements:
- Extend bot functionality with advanced search, improved error handling, and more secure session management.

**Feel free to contribute or suggest features via pull requests!**
