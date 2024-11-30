#pragma once

namespace db::sql {

inline constexpr const char* kGetUser{R"~(
SELECT * FROM users WHERE username = $1
)~"};

inline constexpr const char* kCreateUser{R"~(
INSERT INTO users (username, master_key_hash, salt_encoded, totp_secret) VALUES ($1, $2, $3, $4)
)~"};

inline constexpr const char* kCreatePassword{R"~(
INSERT INTO passwords (user_id, service, login, password_encrypted) VALUES ($1, $2, $3, $4)
)~"};

inline constexpr const char* kGetPassword{R"~(
SELECT * FROM passwords WHERE id = $1 AND user_id = $
)~"};

inline constexpr const char* kGetPasswords{R"~(
SELECT * FROM passwords WHERE user_id = $1
)~"};

inline constexpr const char* kDeletePassword{R"~(
DELETE FROM passwords WHERE id = $1 AND user_id = $
)~"};

}  // namespace db::sql
