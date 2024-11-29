import pytest
import psycopg2
import pyotp
import requests

BASE_URL = "http://localhost:8080/api/v1"

DB_CONFIG = {
    "dbname": "vaulty",
    "user": "vaulty",
    "password": "password",
    "host": "localhost",
    "port": 35432,
}

TRUNCATE_TABLES_SQL = """
TRUNCATE TABLE passwords RESTART IDENTITY CASCADE;
TRUNCATE TABLE users RESTART IDENTITY CASCADE;
"""

USERS = [
    {"username": "svinokrys2000"},
    {"username": "tech_master"},
]

PASSWORDS_FOR_USERS = {
    "svinokrys2000": [
        {"service": "eldom", "login": "kamila", "password": "123456"},
        {"service": "docmed", "login": "kam-sai", "password": "987654"},
    ],
    "tech_master": [
        {"service": "gitlab", "login": "tech_admin", "password": "secure123"},
        {"service": "slack", "login": "tech_team", "password": "team2024"},
    ],
}

@pytest.fixture(autouse=True)
def clean_tables():
    """Очищает таблицы после каждого теста."""
    connection = psycopg2.connect(**DB_CONFIG)
    cursor = connection.cursor()
    cursor.execute(TRUNCATE_TABLES_SQL)
    connection.commit()
    cursor.close()
    connection.close()


@pytest.fixture
def users():
    return USERS


@pytest.fixture
def test_user():
    return USERS[0]


@pytest.fixture
def invalid_jwt():
    return "Bearer invalid.token.signature"


@pytest.fixture
def passwords_for_users():
    return PASSWORDS_FOR_USERS


@pytest.fixture
def test_passwords():
    return PASSWORDS_FOR_USERS["svinokrys2000"]


def user_registration_and_login(user):
    # Регистрация пользователя
    response = requests.post(f"{BASE_URL}/user", json=user)
    assert response.status_code == 200
    data = response.json()
    assert data["message"] == "User registered successfully"
    assert "master_key" in data
    assert "totp_secret" in data

    master_key = data["master_key"]
    totp_secret = data["totp_secret"]
    totp_code = pyotp.TOTP(totp_secret).now()  # Предполагаем, что используем этот код

    # Успешный логин
    login_payload = {**user, "master_key": master_key, "totp_code": totp_code}
    response = requests.post(f"{BASE_URL}/auth", json=login_payload)
    assert response.status_code == 200
    login_data = response.json()
    assert "token" in login_data

    return master_key, totp_secret, login_data["token"]


def multiple_user_registration_and_login(users):
    registered_users = []

    for user in users:
        # Регистрация
        response = requests.post(f"{BASE_URL}/user", json=user)
        assert response.status_code == 200
        data = response.json()
        assert data["message"] == "User registered successfully"
        assert "master_key" in data
        assert "totp_secret" in data

        master_key = data["master_key"]
        totp_secret = data["totp_secret"]
        registered_users.append({"username": user["username"], "master_key": master_key, "totp_secret": totp_secret})

    return registered_users


def test_user_registration_and_login(test_user):
    user_registration_and_login(test_user)


def test_invalid_master_key(test_user):
    # Регистрация и логин
    master_key, totp_secret, token = user_registration_and_login(test_user)

    # Пытаемся залогиниться с неверным мастер-ключом
    totp_code = pyotp.TOTP(totp_secret).now()
    invalid_payload = {**test_user, "master_key": "invalid_key", "totp_code": totp_code}
    response = requests.post(f"{BASE_URL}/auth", json=invalid_payload)
    assert response.status_code == 401
    data = response.json()
    assert data["message"] == "Invalid master key or TOTP code"


def test_invalid_totp_code(test_user):
     # Регистрация и логин
    master_key, totp_secret, token = user_registration_and_login(test_user)

    # Пытаемся залогиниться с неверным мастер-ключом
    totp_code = "123456"
    invalid_payload = {**test_user, "master_key": "invalid_key", "totp_code": totp_code}
    response = requests.post(f"{BASE_URL}/auth", json=invalid_payload)
    assert response.status_code == 401
    data = response.json()
    assert data["message"] == "Invalid master key or TOTP code"


def test_add_password_with_invalid_jwt(invalid_jwt, test_passwords):
    # Добавляем пароль с неверным JWT
    response = requests.post(
        f"{BASE_URL}/password",
        headers={"Authorization": invalid_jwt},
        json=test_passwords[0],
    )
    assert response.status_code == 401
    data = response.json()
    assert data["message"] == "Invalid JWT signature"


def test_add_and_get_passwords(test_user, test_passwords):
    # Регистрация и логин
    master_key, totp_secret, token = user_registration_and_login(test_user)

    # Добавляем пароли
    for password in test_passwords:
        response = requests.post(
            f"{BASE_URL}/password",
            headers={"Authorization": f"Bearer {token}"},
            json=password,
        )
        assert response.status_code == 200
        data = response.json()
        assert data["message"] == "Password added successfully"

    # Получаем список всех паролей
    response = requests.get(
        f"{BASE_URL}/passwords",
        headers={"Authorization": f"Bearer {token}"},
    )
    assert response.status_code == 200
    passwords_data = response.json()
    assert len(passwords_data) >= len(test_passwords)

    for password in test_passwords:
        assert any(
            p["service"] == password["service"] and p["login"] == password["login"] and p["password"] == password["password"]
            for p in passwords_data
        )


def test_get_specific_password(test_user, test_passwords):
    # Регистрация и логин
    master_key, totp_secret, token = user_registration_and_login(test_user)

    # Добавляем пароль
    response = requests.post(
        f"{BASE_URL}/password",
        headers={"Authorization": f"Bearer {token}"},
        json=test_passwords[0],
    )
    assert response.status_code == 200

    # Получаем добавленный пароль по ID
    response = requests.get(
        f"{BASE_URL}/password/1",  # Предполагаем, что это первый добавленный пароль
        headers={"Authorization": f"Bearer {token}"},
    )
    assert response.status_code == 200
    data = response.json()
    assert data["service"] == test_passwords[0]["service"]
    assert data["login"] == test_passwords[0]["login"]
    assert data["password"] == test_passwords[0]["password"]


def test_get_password_with_invalid_jwt(invalid_jwt):
    # Пытаемся получить пароли с неверным JWT
    response = requests.get(
        f"{BASE_URL}/passwords",
        headers={"Authorization": invalid_jwt},
    )
    assert response.status_code == 401
    data = response.json()
    assert data["message"] == "Invalid JWT signature"


def test_add_password_without_auth(test_passwords):
    # Пытаемся добавить пароль без токена
    response = requests.post(
        f"{BASE_URL}/password",
        json=test_passwords[0],
    )
    assert response.status_code == 401
    data = response.json()
    assert data["message"] == "Invalid Authorization header"


def test_get_nonexistent_password(test_user):
    # Регистрация и логин
    master_key, totp_secret, token = user_registration_and_login(test_user)

    # Пытаемся получить пароль с несуществующим ID
    response = requests.get(
        f"{BASE_URL}/password/9999",  # Несуществующий ID
        headers={"Authorization": f"Bearer {token}"},
    )
    assert response.status_code == 404
    data = response.json()
    assert data["message"] == "Password not found"


def test_multiple_user_registration_and_login(users):
    multiple_user_registration_and_login(users)


def test_password_isolation_between_users(users, passwords_for_users):
    # Регистрация и логин для всех пользователей
    registered_users = multiple_user_registration_and_login(users)

    tokens = {}
    for user in registered_users:
        # Логин
        totp_code = pyotp.TOTP(user["totp_secret"]).now()
        response = requests.post(
            f"{BASE_URL}/auth",
            json={"username": user["username"], "master_key": user["master_key"], "totp_code": totp_code},
        )
        assert response.status_code == 200
        data = response.json()
        tokens[user["username"]] = data["token"]

    # Добавление паролей для каждого пользователя
    for username, passwords in passwords_for_users.items():
        token = tokens[username]
        for password in passwords:
            response = requests.post(
                f"{BASE_URL}/password",
                headers={"Authorization": f"Bearer {token}"},
                json=password,
            )
            assert response.status_code == 200
            data = response.json()
            assert data["message"] == "Password added successfully"

    # Проверка паролей для каждого пользователя
    for username, passwords in passwords_for_users.items():
        token = tokens[username]

        # Получаем список паролей
        response = requests.get(
            f"{BASE_URL}/passwords",
            headers={"Authorization": f"Bearer {token}"},
        )
        assert response.status_code == 200
        data = response.json()
        assert len(data) >= len(passwords)

        for password in passwords:
            assert any(
                p["service"] == password["service"] and p["login"] == password["login"] and p["password"] == password["password"]
                for p in data
            )

        # Проверка, что пользователь не видит чужие пароли
        other_users = [u for u in passwords_for_users.keys() if u != username]
        for other_user in other_users:
            for other_password in passwords_for_users[other_user]:
                assert not any(
                    p["service"] == other_password["service"] and p["login"] == other_password["login"] and p["password"] == other_password["password"]
                    for p in data
                )


def test_access_other_users_passwords(users, passwords_for_users):
    # Регистрация и логин для всех пользователей
    registered_users = multiple_user_registration_and_login(users)

    tokens = {}
    for user in registered_users:
        # Логин
        totp_code = pyotp.TOTP(user["totp_secret"]).now()
        response = requests.post(
            f"{BASE_URL}/auth",
            json={"username": user["username"], "master_key": user["master_key"], "totp_code": totp_code},
        )
        assert response.status_code == 200
        data = response.json()
        tokens[user["username"]] = data["token"]

    # Добавление паролей для первого пользователя
    user1 = users[0]["username"]
    token1 = tokens[user1]
    for password in passwords_for_users[user1]:
        response = requests.post(
            f"{BASE_URL}/password",
            headers={"Authorization": f"Bearer {token1}"},
            json=password,
        )
        assert response.status_code == 200

    # Попытка получить доступ ко второму пользователю
    user2 = users[1]["username"]
    token2 = tokens[user2]

    response = requests.get(
        f"{BASE_URL}/passwords",
        headers={"Authorization": f"Bearer {token2}"},
    )
    assert response.status_code == 200
    data = response.json()

    # Убедимся, что пароли первого пользователя недоступны
    for password in passwords_for_users[user1]:
        assert not any(
            p["service"] == password["service"] and p["login"] == password["login"]
            for p in data
        )
