# Password manager

Simple C++ service that uses [userver framework](https://github.com/userver-framework/userver) with PostgreSQL.


## Build from source

```
mkdir build 
cd build
cmake ..
cmake --build . -- -j $nproc
```

## Run service

Fill the `.env` file with corresponding variables

```
POSTGRES_USER=password_manager
POSTGRES_PASSWORD=<your password>
POSTGRES_DB=password_manager
POSTGRES_URL=postgres://${POSTGRES_USER}:${POSTGRES_PASSWORD}@postgres:5432/${POSTGRES_DB}
JWT_SECRET_KEY=<your secret>
CRYPTO_AES_256_BASE64_KEY=<base 64 encoded AES 256 key>
```

Then run the following command
```
docker compose up -d --build
```

Plans:

1. Telegram bot            (HARD)
2. TOTP support            (HARD) 
3. Delete password handler (EASY)



