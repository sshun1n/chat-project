# Secure Terminal Chat (UI) 🔒

Реализация безопасного терминального чата с шифрованием AES-256 и поддержкой кириллицы.

## Установка ⚙️

### Зависимости
```bash
# Ubuntu/Debian
sudo apt install libncurses-dev libssl-dev build-essential
```

### Сборка
```bash
git clone https://github.com/yourusername/secure-chat.git

# Сборка клиента
cd client && make

# Сборка сервера
cd ../server && make
```
graph TD
    Client[Клиент] -->|Зашифрованные данные| Server[Сервер]
    Server -->|Шифрование AES| Database[(Хранилище)]
    Server -->|Бродкаст| Client
