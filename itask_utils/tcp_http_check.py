import socket
import requests
import time
import sys
from urllib.parse import urlparse

def send_tcp_without_http(ip, port=80, data="Test TCP packet", timeout=5):
    """
    Отправка чистого TCP-пакета без HTTP
    """
    try:
        # Создаем TCP-сокет
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Подключаемся к серверу
        sock.connect((ip, port))
        print(f"[+] Успешное TCP-подключение к {ip}:{port}")
        
        # Отправляем данные
        if data:
            sock.sendall(data.encode())
            print(f"[+] Отправлено {len(data)} байт данных")
        
        # Пытаемся получить ответ (необязательно)
        try:
            response = sock.recv(1024)
            if response:
                print(f"[+] Получен ответ: {response[:100]}...")
        except socket.timeout:
            print("[i] Таймаут при ожидании ответа")
        
        sock.close()
        return True
        
    except socket.error as e:
        print(f"[-] Ошибка TCP: {e}")
        return False

def send_tcp_with_http(ip, port=80, path="/", method="GET", timeout=5):
    """
    Отправка HTTP-запроса поверх TCP
    """
    try:
        # Формируем URL
        if not ip.startswith("http"):
            url = f"http://{ip}:{port}{path}"
        else:
            url = ip
        
        print(f"[+] Отправка HTTP {method} запроса на {url}")
        
        # Отправляем HTTP-запрос
        if method.upper() == "GET":
            response = requests.get(url, timeout=timeout, verify=False)
        elif method.upper() == "POST":
            response = requests.post(url, timeout=timeout, verify=False)
        elif method.upper() == "HEAD":
            response = requests.head(url, timeout=timeout, verify=False)
        else:
            print(f"[-] Неподдерживаемый метод: {method}")
            return False
        
        print(f"[+] HTTP статус: {response.status_code}")
        print(f"[+] Ответ сервера (первые 200 символов):")
        print(response.text[:200])
        
        return True
        
    except requests.exceptions.RequestException as e:
        print(f"[-] Ошибка HTTP: {e}")
        return False

def send_raw_tcp_packet(ip, port=80, payload=None):
    """
    Отправка сырого TCP-пакета (более низкоуровневый)
    """
    try:
        # Создаем сырой сокет (требует прав администратора на Windows/Linux)
        sock = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
        
        # Формируем базовый TCP-заголовок
        source_port = 54321  # Произвольный порт источника
        seq_num = 1000
        ack_num = 0
        data_offset = 5  # 5 * 4 = 20 байт (стандартный размер TCP заголовка)
        
        # TCP флаги: SYN=2, ACK=16 и т.д.
        tcp_flags = 0
        
        if not payload:
            payload = b"Raw TCP packet data"
        
        # Формируем простой TCP пакет
        # В реальном приложении здесь была бы полная сборка TCP заголовка
        packet = payload
        
        sock.sendto(packet, (ip, port))
        print(f"[+] Сырой TCP-пакет отправлен на {ip}:{port}")
        sock.close()
        
    except PermissionError:
        print("[-] Требуются права администратора/root для отправки сырых пакетов")
        return False
    except Exception as e:
        print(f"[-] Ошибка: {e}")
        return False

def main():
    print("=" * 50)
    print("TCP Packet Sender")
    print("=" * 50)
    
    # Получаем IP от пользователя
    ip = input("Введите IP адрес или URL: ").strip()
    if not ip:
        print("[-] IP адрес обязателен")
        return
    
    # Извлекаем IP из URL если нужно
    if ip.startswith("http"):
        parsed = urlparse(ip)
        ip = parsed.hostname
        port = parsed.port if parsed.port else 80
        path = parsed.path if parsed.path else "/"
    else:
        port_input = input("Введите порт (по умолчанию 80): ").strip()
        port = int(port_input) if port_input else 80
        path = "/"
    
    while True:
        print("\n" + "=" * 50)
        print("Выберите тип пакета:")
        print("1. TCP без HTTP (простое подключение)")
        print("2. TCP с HTTP GET запросом")
        print("3. TCP с HTTP POST запросом")
        print("4. TCP с HTTP HEAD запросом")
        print("5. Отправка сырого TCP пакета (требует прав админа)")
        print("6. Выход")
        
        choice = input("Ваш выбор (1-6): ").strip()
        
        if choice == "1":
            data = input("Введите данные для отправки (Enter для стандартных): ").strip()
            if not data:
                data = "Test TCP data without HTTP"
            send_tcp_without_http(ip, port, data)
            
        elif choice == "2":
            custom_path = input(f"Введите путь (по умолчанию {path}): ").strip()
            if custom_path:
                path = custom_path
            send_tcp_with_http(ip, port, path, "GET")
            
        elif choice == "3":
            custom_path = input(f"Введите путь (по умолчанию {path}): ").strip()
            if custom_path:
                path = custom_path
            send_tcp_with_http(ip, port, path, "POST")
            
        elif choice == "4":
            custom_path = input(f"Введите путь (по умолчанию {path}): ").strip()
            if custom_path:
                path = custom_path
            send_tcp_with_http(ip, port, path, "HEAD")
            
        elif choice == "5":
            print("[!] Для этой функции требуются права администратора")
            confirm = input("Продолжить? (y/n): ").strip().lower()
            if confirm == 'y':
                payload_input = input("Введите данные в hex или текст (Enter для стандартных): ").strip()
                if payload_input:
                    if all(c in "0123456789abcdefABCDEF" for c in payload_input.replace(" ", "")):
                        # Hex данные
                        payload = bytes.fromhex(payload_input.replace(" ", ""))
                    else:
                        # Текстовые данные
                        payload = payload_input.encode()
                else:
                    payload = b"Raw TCP packet test data"
                
                send_raw_tcp_packet(ip, port, payload)
                
        elif choice == "6":
            print("[+] Выход из программы")
            break
            
        else:
            print("[-] Неверный выбор")
        
        # Пауза между запросами
        time.sleep(1)

if __name__ == "__main__":
    # Предупреждение о безопасности
    print("[!] ПРЕДУПРЕЖДЕНИЕ: Используйте этот скрипт только для легального тестирования!")
    print("[!] Отправка пакетов на чужие системы без разрешения незаконна!\n")
    
    # Для отправки сырых пакетов на Windows может потребоваться запуск от администратора
    main()
