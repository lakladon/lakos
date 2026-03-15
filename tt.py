import re
import os

def clean_content(text, is_c_style=False):
    if is_c_style:
        # Удаляем многострочные /* ... */
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
        # Удаляем однострочные //
        text = re.sub(r'//.*$', '', text, flags=re.MULTILINE)
    
    # Удаляем комментарии ASM и Makefile (; и #)
    text = re.sub(r'[ \t]*[;#].*$', '', text, flags=re.MULTILINE)
    
    # Чистим пустые строки
    return re.sub(r'^\s*$\n', '', text, flags=re.MULTILINE)

def process_directory():
    # Расширения для ASM/Makefile и для C/C++
    asm_ext = ('.asm', '.s', '.inc', 'makefile')
    c_ext = ('.c', '.cpp', '.h', '.hpp')

    for root, dirs, files in os.walk('.'):
        for file in files:
            lower_file = file.lower()
            file_path = os.path.join(root, file)
            
            # Пропускаем сам скрипт
            if file == 'clean_all.py': continue

            is_c = any(lower_file.endswith(ext) for ext in c_ext)
            is_asm = any(ext in lower_file for ext in asm_ext)

            if is_c or is_asm:
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()

                    cleaned = clean_content(content, is_c_style=is_c)

                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(cleaned)
                    print(f"Очищен: {file_path}")
                except Exception as e:
                    print(f"Ошибка в файле {file_path}: {e}")

if __name__ == "__main__":
    if input("Очистить все C, ASM и Makefile в этой папке? (y/n): ").lower() == 'y':
        process_directory()
        print("Готово.")
