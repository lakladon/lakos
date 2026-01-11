import pygame
import sys
import logging

# Настройка логирования
logging.basicConfig(filename='mouse_test.log', level=logging.INFO,
                    format='%(asctime)s - %(message)s')

def mouse_test():
    # Инициализация Pygame
    pygame.init()

    # Размеры окна
    WIDTH, HEIGHT = 800, 600
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("Mouse Test Application")

    # Цвета
    BLACK = (0, 0, 0)
    WHITE = (255, 255, 255)
    RED = (255, 0, 0)

    # Начальные координаты мыши
    mouse_x, mouse_y = WIDTH // 2, HEIGHT // 2
    cursor_size = 20

    # Скорость движения
    speed = 5

    # Шрифт для текста
    font = pygame.font.Font(None, 36)

    # Главный цикл
    running = True
    while running:
        screen.fill(BLACK)

        # Обработка событий
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
            elif event.type == pygame.MOUSEBUTTONDOWN:
                # Логирование клика
                button = "Left" if event.button == 1 else "Right" if event.button == 3 else f"Button {event.button}"
                logging.info(f"Mouse clicked: {button} at ({event.pos[0]}, {event.pos[1]})")
                print(f"Mouse clicked: {button} at ({event.pos[0]}, {event.pos[1]})")

        # Обработка клавиш для движения
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            mouse_x -= speed
        if keys[pygame.K_RIGHT]:
            mouse_x += speed
        if keys[pygame.K_UP]:
            mouse_y -= speed
        if keys[pygame.K_DOWN]:
            mouse_y += speed

        # Обработка реального движения мыши
        real_mouse_x, real_mouse_y = pygame.mouse.get_pos()
        if abs(real_mouse_x - mouse_x) > 10 or abs(real_mouse_y - mouse_y) > 10:
            mouse_x = real_mouse_x
            mouse_y = real_mouse_y
            logging.info(f"Mouse moved to ({mouse_x}, {mouse_y})")
            print(f"Mouse moved to ({mouse_x}, {mouse_y})")

        # Ограничение координат в пределах экрана
        try:
            if mouse_x < 0:
                mouse_x = 0
                logging.warning("Mouse tried to go left of screen")
            if mouse_x > WIDTH - cursor_size:
                mouse_x = WIDTH - cursor_size
                logging.warning("Mouse tried to go right of screen")
            if mouse_y < 0:
                mouse_y = 0
                logging.warning("Mouse tried to go above screen")
            if mouse_y > HEIGHT - cursor_size:
                mouse_y = HEIGHT - cursor_size
                logging.warning("Mouse tried to go below screen")
        except Exception as e:
            logging.error(f"Exception in coordinate handling: {e}")
            print(f"Exception in coordinate handling: {e}")

        # Рисование курсора (простой красный квадрат)
        pygame.draw.rect(screen, RED, (mouse_x, mouse_y, cursor_size, cursor_size))

        # Отображение координат
        coord_text = font.render(f"X: {mouse_x}, Y: {mouse_y}", True, WHITE)
        screen.blit(coord_text, (10, 10))

        # Обновление экрана
        pygame.display.flip()

        # Задержка для плавности
        pygame.time.Clock().tick(60)

    # Завершение
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    mouse_test()