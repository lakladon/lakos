import re
import os

def clean_asm_makefile(text):
    # В ASM и Makefile ; и # — это комментарии
    text = re.sub(r'[ \t]*[;#].*$', '', text, flags=re.MULTILINE)
    return text

def clean_c_style(text):
    # 1. Удаляем многострочные /* ... */
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # 2. Удаляем однострочные // (но НЕ трогаем ;) 
    text = re.sub(r'//.*$', '', text, flags=re.MULTILINE)
    return text

def process_directory():
    asm_ext = ('.asm', '.s', '.inc', 'makefile')
    c_ext = ('.c', '.cpp', '.h', '.hpp')

    for root, dirs, files in os.walk('.'):
        for file in files:
            file_path = os.path.join(root, file)
            if file == 'clean_all.py': continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                if any(ext in file.lower() for ext in asm_ext):
                    cleaned = clean_asm_makefile(content)
                elif any(file.lower().endswith(ext) for ext in c_ext):
                    cleaned = clean_c_style(content)
                else:
                    continue

                # Финальная чистка пустых строк для всех
                cleaned = re.sub(r'^\s*$\n', '', cleaned, flags=re.MULTILINE)

                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(cleaned)
                print(f"Очищен: {file_path}")
            except Exception as e:
                print(f"Ошибка {file_path}: {e}")

if __name__ == "__main__":
    if input("Очистить код БЕЗ удаления точек с запятой в C? (y/n): ").lower() == 'y':
        process_directory()
