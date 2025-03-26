# Secure Terminal Chat 🔒

[![Build Status](https://img.shields.io/github/actions/workflow/status/sshun1n/secure-chat/build.yml)](https://github.com/sshun1n/secure-chat/actions)

Реализация безопасного терминального чата с шифрованием AES-256 и поддержкой кириллицы.

## Установка ⚙️

### Зависимости
```bash
# Ubuntu/Debian
sudo apt install libncursesw5-dev libssl-dev
```

### Сборка
```bash
git clone https://github.com/yourusername/secure-chat.git

# Сборка клиента
cd client && make

# Сборка сервера
cd ../server && make
```
