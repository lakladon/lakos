import re
import os

def clean_c_code(text):
    # 1. Удаляем многострочные комментарии /* ... */
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # 2. Удаляем однострочные комментарии //, но НЕ внутри кавычек
    # Это регулярное выражение ищет кавычки (и пропускает их) или //
    text = re.sub(r'("(?:\\.|[^"\\])*")|//.*', lambda m: m.group(1) if m.group(1) else "", text)
    return text

def clean_asm_makefile(text):
    # Удаляем ; и #, но сохраняем структуру строк (насколько это возможно для ASM)
    text = re.sub(r'("(?:\\.|[^"\\])*")|[;#].*', lambda m: m.group(1) if m.group(1) else "", text)
    return text

def process():
    for root, dirs, files in os.walk('.'):
        for file in files:
            path = os.path.join(root, file)
            ext = file.lower()
            
            if file == 'safe_clean.py': continue

            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                if ext.endswith(('.c', '.h', '.cpp', '.hpp')):
                    cleaned = clean_c_code(content)
                elif ext.endswith(('.asm', '.s', '.inc')) or 'makefile' in ext:
                    cleaned = clean_asm_makefile(content)
                else:
                    continue

                # Удаляем лишние пустые строки (оставляем максимум одну подряд)
                cleaned = re.sub(r'\n\s*\n', '\n\n', cleaned)

                with open(path, 'w', encoding='utf-8') as f:
                    f.write(cleaned)
                print(f"DONE: {path}")
            except Exception as e:
                print(f"ERROR {path}: {e}")

if __name__ == "__main__":
    process()
