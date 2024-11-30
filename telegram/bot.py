from telegram import Update, ReplyKeyboardRemove
from telegram.ext import (
    ApplicationBuilder,
    CommandHandler,
    MessageHandler,
    ContextTypes,
    filters,
    ConversationHandler,
)
import requests
import os
import random
import redis
import string
from datetime import datetime
from dateutil import parser
from typing import Optional

BASE_URL = "http://vaulty_service:8080/api/v1"
DEFAULT_PASSWORD_LENGTH = 16
DEFAULT_TOKEN_TTL = 7 * 24 * 60 * 60 # 1 week
LOGIN, MASTER_KEY = range(2)
CONFIRM_RESET, AWAITING_TOTP = range(2)
AWAITING_PASSWORD = range(1)

REDIS_CONN = None


def get_redis_connection():
    global REDIS_CONN
    if REDIS_CONN is None:
        redis_host = os.getenv("REDIS_HOST", "localhost")
        redis_port = int(os.getenv("REDIS_PORT", 6379))
        redis_password = os.getenv("REDIS_PASSWORD", None)
        REDIS_CONN = redis.StrictRedis(
            host=redis_host,
            port=redis_port,
            password=redis_password,
            decode_responses=True,
        )
    return REDIS_CONN


def add_token(user_id: int, token: str, ttl: int) -> None:
    redis_conn = get_redis_connection()
    redis_conn.setex(f"user:{user_id}:token", ttl, token)


def get_token(user_id: int) -> Optional[str]:
    redis_conn = get_redis_connection()
    return redis_conn.get(f"user:{user_id}:token")


def delete_token(user_id: int) -> None:
    redis_conn = get_redis_connection()
    redis_conn.delete(f"user:{user_id}:token")


def set_token_ttl(user_id: int, ttl: int) -> bool:
    redis_conn = get_redis_connection()
    return redis_conn.expire(f"user:{user_id}:token", ttl)


def escape_markdown_v2(text: str) -> str:
    """Экранирование всех специальных символов для MarkdownV2"""
    special_chars = ['_', '*', '[', ']', '(', ')', '~', '`', '>', '#', '+', '-', '=', '|', '{', '}', '.', '!', ':']
    for char in special_chars:
        text = text.replace(char, f"\\{char}")
    return text


async def cmd_help(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    await update.message.reply_text(
    "📖 *Помощь*\n\n"
    "🔐 *Доступные команды:*\n"
    "/start - Регистрация или вход в бота.\n"
    "/add - Добавить новый пароль.\n"
    "/del <ID> - Удилть пароль.\n"
    "/get - Получить сохранённые пароли.\n"
    "/logout - Выйти из аккаунта и очистить сессию.\n"
    "/gen <длина> - Сгенерировать надёжный пароль. По умолчанию длина 16 символов. Минимум — 8 символов.\n"
    "/reset\_user\_forever - 🚨 *ВНИМАНИЕ: Эта команда удалит ваш аккаунт и ВСЕ сохранённые пароли НАВСЕГДА!* 🚨\n"
    "/help - Показать это сообщение.\n\n"
    "🔑 *Как настроить Google Authenticator:*\n"
    "1. Откройте приложение Google Authenticator.\n"
    "2. Нажмите на значок '+' для добавления нового аккаунта.\n"
    "3. Выберите 'Ввести ключ вручную'.\n"
    "4. Введите предоставленный TOTP-ключ и укажите имя vaulty.\n"
    "5. Нажмите 'Добавить', чтобы завершить настройку.\n"
    "6. Используйте сгенерированные одноразовые коды для авторизации в боте.\n\n"
    "🛡 *Рекомендации по безопасности:*\n"
    "Рекомендуем настроить *автоудаление* диалога с ботом через 24 часа.\n"
    "Это можно сделать в настройках чата:\n"
    "1. Откройте этот чат.\n"
    "2. Нажмите на название чата сверху.\n"
    "3. Выберите 'Автоудаление сообщений'.\n"
    "4. Установите время автоудаления на 24 часа.\n\n"
    "❗ Это поможет защитить ваши данные в случае утраты доступа к вашему устройству.", 
    parse_mode="Markdown")


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id
    username = str(user_id)

    response = requests.post(f"{BASE_URL}/user", json={"username": username})

    if response.status_code == 200:
        data = response.json()
        master_key = data["master_key"]
        totp_secret = data["totp_secret"]

        await update.message.reply_text(
            "👋 Добро пожаловать! Вы успешно зарегистрированы.\n"
            "⚠️ *Сохраните ваш мастер-ключ и TOTP-ключ*, они понадобятся для авторизации.",
            parse_mode="Markdown",
        )
        await update.message.reply_text(
            f"🔑 *Ваш мастер-ключ:*",
            parse_mode="Markdown",
        )
        await update.message.reply_text(
            f"||{escape_markdown_v2(master_key)}||",
            parse_mode="MarkdownV2",
        )
        await update.message.reply_text(
            f"📲 *Ваш TOTP-ключ:*",
            parse_mode="Markdown",
        )
        await update.message.reply_text(
            f"||{escape_markdown_v2(totp_secret)}||",
            parse_mode="MarkdownV2",
        )

        await update.message.reply_text(
            "👋 Для продолжения установите приложение *Google Authenticator*:\n"
            "[🍎 Скачать для iOS](https://apps.apple.com/app/google-authenticator/id388497605) | "
            "[🤖 Скачать для Android](https://play.google.com/store/apps/details?id=com.google.android.apps.authenticator2)\n\n"
            "📖 Подробный гайд по настройке доступен в команде /help.\n"
            "🔄 После настройки вернитесь и нажмите /start, чтобы продолжить.",
            parse_mode="Markdown",
        )

        return ConversationHandler.END

    else:
        await update.message.reply_text(
            "📝 Введите ваш *мастер-ключ* и *TOTP-код* через пробел (пример: `ключ код`) для авторизации.",
            parse_mode="Markdown",
        )
        return MASTER_KEY


async def authenticate(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id
    try:
        master_key, totp_code = update.message.text.split(" ", 2)
    except:
        await update.message.reply_text(
            "📝 Введите ваш *мастер-ключ* и *TOTP-код* через пробел (пример: `ключ код`) для авторизации.",
            parse_mode="Markdown",
        )
        return MASTER_KEY

    payload = {"username": str(user_id), "master_key": master_key, "totp_code": totp_code}
    response = requests.post(f"{BASE_URL}/auth", json=payload)

    if response.status_code == 200:
        token = response.json()["token"]
        add_token(user_id, token, DEFAULT_TOKEN_TTL);
        await update.message.reply_text(
            "✅ *Авторизация прошла успешно!* 🎉\n"
            "Теперь вы можете использовать команды:\n"
            "• /add — добавить пароль\n"
            "• /get — получить сохранённые пароли\n"
            "• /del — удалить пароль\n"
            "• /logout — выйти из аккаунта.",
            parse_mode="Markdown",
        )
        return ConversationHandler.END
    else:
        await update.message.reply_text(
            "❌ *Ошибка авторизации*. Попробуйте снова.\n"
            "Убедитесь, что ввели правильный мастер-ключ и TOTP-код.",
            parse_mode="Markdown",
        )
        return ConversationHandler.END
        

async def cmd_add_password(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    """Начинает процесс добавления пароля."""
    user_id = update.message.from_user.id
    token = get_token(user_id)
    if token is None:
        await update.message.reply_text(
            "🔒 *Сначала авторизуйтесь!* Используйте команду /start для входа.",
            parse_mode="Markdown",
        )
        return ConversationHandler.END

    await update.message.reply_text(
        "📝 Введите название сервиса, логин и пароль через пробел.\n"
        "Пример: `сервис логин пароль`",
        parse_mode="Markdown",
    )
    return AWAITING_PASSWORD


async def handle_add_password(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    """Обрабатывает ввод пользователя и добавляет пароль в систему."""
    user_id = update.message.from_user.id
    try:
        service, login, password = update.message.text.split(" ", 2)
    except ValueError:
        await update.message.reply_text(
            "⚠️ Пожалуйста, введите данные в формате:\n"
            "`сервис логин пароль`",
            parse_mode="Markdown",
        )
        return AWAITING_PASSWORD

    token = get_token(user_id)
    payload = {"service": service, "login": login, "password": password}
    response = requests.post(
        f"{BASE_URL}/password",
        headers={"Authorization": f"Bearer {token}"},
        json=payload,
    )

    if response.status_code == 200:
        await update.message.reply_text(
            "✅ *Пароль успешно добавлен!* 🔐",
            parse_mode="Markdown",
        )
    else:
        await update.message.reply_text(
            "❌ *Не удалось добавить пароль.* Попробуйте позже.",
            parse_mode="Markdown",
        )
    return ConversationHandler.END


async def add_password_cancel(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    """Отмена операции добавления пароля."""
    await update.message.reply_text(
        "❌ *Операция отменена.*",
        parse_mode="Markdown",
    )
    return ConversationHandler.END


async def cmd_delete_password(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    token = get_token(user_id)
    if token is None:
        await update.message.reply_text(
            "🔒 *Сначала авторизуйтесь!* Используйте команду /start для входа.",
            parse_mode="Markdown",
        )
        return
    
    if not context.args:
        await update.message.reply_text(
            "⚠️ Пожалуйста, укажите ID пароля, который нужно удалить.\n"
            "Пример: `/del <ID пароля>`",
            parse_mode="Markdown",
        )
        return
    
    add_token(user_id, token, DEFAULT_TOKEN_TTL)
    password_id = context.args[0]

    response = requests.delete(
        f"{BASE_URL}/password/{password_id}",
        headers={"Authorization": f"Bearer {token}"},
    )

    if response.status_code == 200:
        await update.message.reply_text(
            f"✅ Пароль с ID `{password_id}` успешно удалён! 🔐",
            parse_mode="Markdown",
        )
    else:
        await update.message.reply_text(
            f"❌ Не удалось удалить пароль с ID `{password_id}`. Попробуйте еще раз.",
            parse_mode="Markdown",
        )


async def handle_message(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    token = get_token(user_id)
    if token is None:
        await update.message.reply_text(
            "🔒 *Сначала авторизуйтесь!* Используйте команду /start для входа.",
            parse_mode="Markdown",
        )
        return

    await update.message.reply_text(
        "🤔 *Неизвестная команда*. Используйте /help чтобы получить список доступных команд.",
        parse_mode="Markdown",
    )


async def cmd_get_passwords(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    token = get_token(user_id)
    if token is None:
        await update.message.reply_text(
            "🔒 *Сначала авторизуйтесь!* Используйте команду /start для входа.",
            parse_mode="Markdown",
        )
        return

    response = requests.get(
        f"{BASE_URL}/passwords",
        headers={"Authorization": f"Bearer {token}"},
    )

    if response.status_code == 200:
        passwords = response.json()
        if passwords:
            for password in passwords:
                updated_at_str = password['updated_at']
                updated_at = parser.isoparse(updated_at_str)
                formatted_date = updated_at.strftime("%d.%m.%Y %H:%M:%S")

                await update.message.reply_text(
                    f"🆔 *ID:* {password['id']}\n"
                    f"🔐 *Сервис:* {password['service']}\n"
                    f"🔑 *Логин:* {password['login']}\n"
                    f"🗓 *Создано:* {formatted_date}\n"
                    f"💻 *Пароль:*",
                    parse_mode="Markdown",
                )
                await update.message.reply_text(
                    f"||{escape_markdown_v2(password['password'])}||",
                    parse_mode="MarkdownV2",
                )
        else:
            await update.message.reply_text(
                "❌ Нет сохранённых паролей.",
                parse_mode="Markdown",
            )
    else:
        await update.message.reply_text(
            "⚠️ Не удалось получить пароли. Попробуйте позже.",
            parse_mode="Markdown",
        )


async def cmd_logout(update: Update, context: ContextTypes.DEFAULT_TYPE):
    user_id = update.message.from_user.id
    token = get_token(user_id)
    if token is not None:
        delete_token(user_id)
        await update.message.reply_text(
            "✅ *Вы успешно вышли из системы.*\nДо новых встреч! 👋",
            parse_mode="Markdown",
        )
    else:
        await update.message.reply_text(
            "❌ *Вы не были авторизованы.*\nПожалуйста, выполните вход с помощью команды /start.",
            parse_mode="Markdown",
        )

async def cmd_reset_user(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    await update.message.reply_text(
            "⚠️ *Внимание*: Это действие удалит *все ваши пароли* и *ваш аккаунт* из базы данных. "
            "Оно необратимо.\n\n"
            "Если вы уверены, напишите `Yes` для подтверждения.",
        parse_mode="Markdown",
    )
    return CONFIRM_RESET


async def reset_user_confirm_reset(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    if update.message.text.strip().lower() == "yes":
        await update.message.reply_text(
            "📲 Введите ваш *TOTP-код* для подтверждения удаления.",
            parse_mode="Markdown",
        )
        return AWAITING_TOTP
    else:
        await update.message.reply_text(
            "❌ *Отмена действия.* Ваши данные остаются в безопасности.",
            parse_mode="Markdown",
        )
        return ConversationHandler.END


async def reset_user_delete_user(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    user_id = update.message.from_user.id
    totp_code = update.message.text.strip()

    payload = {"username": str(user_id), "totp_code": totp_code}
    response = requests.delete(f"{BASE_URL}/user", json=payload)

    if response.status_code == 200:
        delete_token(user_id)
        await update.message.reply_text(
            "✅ *Ваш аккаунт и пароли успешно удалены.*\nМы будем ждать вас снова!",
            parse_mode="Markdown",
        )
    else:
        await update.message.reply_text(
            "❌ *Не удалось удалить аккаунт.* Проверьте ваш TOTP-код и повторите попытку.",
            parse_mode="Markdown",
        )
    
    return ConversationHandler.END


async def reset_user_cancel(update: Update, context: ContextTypes.DEFAULT_TYPE) -> int:
    await update.message.reply_text(
        "❌ *Отмена действия.* Ваши данные остаются в безопасности.",
        parse_mode="Markdown",
    )
    return ConversationHandler.END


def generate_password(length: int = DEFAULT_PASSWORD_LENGTH) -> str:
    if length < 8:
        raise ValueError("Password length must be at least 8 characters")
    
    if length > 128:
        raise ValueError("Password length must be at max 128 characters")

    letters = string.ascii_letters
    digits = string.digits
    symbols = "!@#$%^&*()-_=+[]{}|;:,.<>?/"

    all_characters = letters + digits + symbols
    password = random.choice(letters) + random.choice(digits) + random.choice(symbols)

    password += ''.join(random.choices(all_characters, k=length - 3))

    password = ''.join(random.sample(password, len(password)))

    return password


async def cmd_generate_password(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    try:
        if context.args:
            length = int(context.args[0])
        else:
            length = DEFAULT_PASSWORD_LENGTH

        password = generate_password(length)

        await update.message.reply_text(
            f"🔑 *Сгенерированный пароль:*",
            parse_mode="Markdown",
        )

        await update.message.reply_text(
            f"||{escape_markdown_v2(password)}||",
            parse_mode="MarkdownV2",
        )

    except ValueError as e:
        await update.message.reply_text(
            f"❌ Неверный ввод: {e}\nПожалуйста, укажите корректную длину пароля (минимум 8 символов).",
            parse_mode="Markdown",
        )


def main():
    token = os.getenv("TELEGRAM_BOT_TOKEN")

    application = ApplicationBuilder().token(token).build()

    start_conv_handler = ConversationHandler(
        entry_points=[CommandHandler("start", start)],
        states={
            MASTER_KEY: [MessageHandler(filters.TEXT, authenticate)],
        },
        fallbacks=[],
    )

    reset_user_conv_handler = ConversationHandler(
        entry_points=[CommandHandler("reset_user_forever", cmd_reset_user)],
        states={
            CONFIRM_RESET: [MessageHandler(filters.TEXT & ~filters.COMMAND, reset_user_confirm_reset)],
            AWAITING_TOTP: [MessageHandler(filters.TEXT & ~filters.COMMAND, reset_user_delete_user)],
        },
        fallbacks=[CommandHandler("cancel", reset_user_cancel)],
    )

    add_password_conv_handler = ConversationHandler(
        entry_points=[CommandHandler("add", cmd_add_password)],
        states={
            AWAITING_PASSWORD: [MessageHandler(filters.TEXT & ~filters.COMMAND, handle_add_password)],
        },
        fallbacks=[CommandHandler("cancel", add_password_cancel)],
    )

    application.add_handler(start_conv_handler)
    application.add_handler(reset_user_conv_handler)
    application.add_handler(add_password_conv_handler)
    application.add_handler(CommandHandler("get", cmd_get_passwords))
    application.add_handler(CommandHandler("del", cmd_delete_password))
    application.add_handler(CommandHandler("logout", cmd_logout))
    application.add_handler(CommandHandler("help", cmd_help))
    application.add_handler(CommandHandler("gen", cmd_generate_password))
    application.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_message))

    application.run_polling()


if __name__ == "__main__":
    main()