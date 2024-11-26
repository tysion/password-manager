from telegram import Update, ReplyKeyboardRemove
from telegram.bot import (
    ApplicationBuilder,
    CommandHandler,
    MessageHandler,
    ContextTypes,
    filters,
    ConversationHandler,
)
import requests
import os

BASE_URL = "http://password_manager:8080/api/v1"
TOKENS = {}
LOGIN, MASTER_KEY = range(2)

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id

    if user_id in TOKENS:
        await update.message.reply_text("Вы уже авторизованы! Используйте команды /add, /get или /logout.")
        return ConversationHandler.END

    await update.message.reply_text(
        "Добро пожаловать! Ваш ID: {user_id}. Для регистрации введите мастер-ключ."
    )
    return MASTER_KEY


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id

    if user_id in TOKENS:
        await update.message.reply_text("Вы уже авторизованы! Используйте команды /add, /get или /logout.")
        return ConversationHandler.END

    await update.message.reply_text(
        "Добро пожаловать! Ваш ID: {user_id}. Для регистрации введите мастер-ключ."
    )
    return MASTER_KEY


async def authenticate(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id
    master_key = update.message.text.strip()

    payload = {"username": str(user_id), "master_key": master_key, "totp_code": 123456}
    response = requests.post(f"{BASE_URL}/auth", json=payload)

    if response.status_code == 200:
        token = response.json()["token"]
        TOKENS[user_id] = token
        await update.message.reply_text("Авторизация прошла успешно! Вы можете использовать команды /add, /get или /logout.")
        return ConversationHandler.END
    else:
        await update.message.reply_text("Ошибка авторизации. Попробуйте снова.")
        return MASTER_KEY
    

async def add_password(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    if user_id not in TOKENS:
        await update.message.reply_text("Сначала авторизуйтесь с помощью команды /start.")
        return

    await update.message.reply_text("Введите название сервиса, логин и пароль через пробел (сервис логин пароль):")
    context.user_data["awaiting_password"] = True


async def handle_message(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Обработка сообщений после команды /add."""
    user_id = update.message.from_user.id
    if user_id not in TOKENS:
        await update.message.reply_text("Сначала авторизуйтесь с помощью команды /start.")
        return

    if context.user_data.get("awaiting_password"):
        try:
            service, login, password = update.message.text.split(" ", 2)
        except ValueError:
            await update.message.reply_text("Пожалуйста, введите данные в формате: сервис логин пароль.")
            return

        token = TOKENS[user_id]
        payload = {"service": service, "login": login, "password": password}
        response = requests.post(
            f"{BASE_URL}/password",
            headers={"Authorization": f"Bearer {token}"},
            json=payload,
        )

        if response.status_code == 200:
            await update.message.reply_text("Пароль успешно добавлен!")
        else:
            await update.message.reply_text("Не удалось добавить пароль.")
        context.user_data["awaiting_password"] = False
    else:
        await update.message.reply_text("Неизвестная команда. Используйте /add, /get или /logout.")


async def get_passwords(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Retrieve all saved passwords for the user."""
    user_id = update.message.from_user.id
    if user_id not in TOKENS:
        await update.message.reply_text("Сначала авторизуйтесь с помощью команды /start.")
        return

    token = TOKENS[user_id]
    response = requests.get(
        f"{BASE_URL}/passwords",
        headers={"Authorization": f"Bearer {token}"},
    )

    if response.status_code == 200:
        passwords = response.json()
        if passwords:
            for password in passwords:
                await update.message.reply_text(
                    f"Сервис: {password['service']}\Логин: {password['login']}"
                )
                await update.message.reply_text(
                    f"Пароль: {password['password']}",
                    protect_content=True,  # Hides the message content
                )
        else:
            await update.message.reply_text("Нет сохранённых паролей.")
    else:
        await update.message.reply_text("Не удалось получить пароли.")


async def logout(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    if user_id in TOKENS:
        del TOKENS[user_id]
        await update.message.reply_text("Вы успешно вышли из системы.")
    else:
        await update.message.reply_text("Вы не были авторизованы.")


def main():
    token = os.getenv("TELEGRAM_BOT_TOKEN")

    application = ApplicationBuilder().token(token).build()

    conv_handler = ConversationHandler(
        entry_points=[CommandHandler("start", start)],
        states={
            MASTER_KEY: [MessageHandler(filters.TEXT, authenticate)],
        },
        fallbacks=[],
    )

    application.add_handler(conv_handler)
    application.add_handler(CommandHandler("add", add_password))
    application.add_handler(CommandHandler("get", get_passwords))
    application.add_handler(CommandHandler("logout", logout))
    application.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_message))

    application.run_polling()


if __name__ == "__main__":
    main()