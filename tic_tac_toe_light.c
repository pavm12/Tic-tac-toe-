/*
 * Крестики-нолики в консоли 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SIZE 30     /* максимальный размер поля */
#define EMPTY    ' '    /* так выглядит пустая клетка */

typedef struct {
    int rows, cols;     /* размер поля */
    int win;            /* сколько символов подряд нужно для победы */
    int moves;          /* сколько клеток уже занято */
    char *cells;        /* клетки поля, хранятся строка за строкой */
} Board;

static int eof_reached = 0;     /* станет 1, если ввод закончился (Ctrl+D) */

/* ---------- ввод ---------- */

/* Читает одну строку с клавиатуры. Возвращает 0, если ввод закончился. */
static int read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL) {
        eof_reached = 1;
        return 0;
    }
    if (strchr(buf, '\n') == NULL) {        /* строка не влезла в буфер: */
        int ch;                             /* выбрасываем остаток */
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    return 1;
}

/* Читает из строки ровно n чисел (1 или 2) и ничего лишнего. */
static int read_numbers(int n, int *a, int *b)
{
    char line[100], extra;

    if (!read_line(line, sizeof line))
        return 0;
    if (n == 1)
        return sscanf(line, "%9d %c", a, &extra) == 1;
    return sscanf(line, "%9d %9d %c", a, b, &extra) == 2;
}

/* Спрашивает число от lo до hi, пока не получит правильное. */
static int ask_number(const char *text, int lo, int hi)
{
    int x;

    while (!eof_reached) {
        printf("%s (%d-%d): ", text, lo, hi);
        if (read_numbers(1, &x, NULL) && x >= lo && x <= hi)
            return x;
        if (!eof_reached)
            printf("Ошибка: нужно ввести целое число от %d до %d.\n", lo, hi);
    }
    return 0;
}

/* ---------- поле ---------- */

static char at(const Board *b, int r, int c)
{
    return b->cells[r * b->cols + c];
}

/* Очищает поле для новой партии. */
static void board_clear(Board *b)
{
    memset(b->cells, EMPTY, (size_t)b->rows * b->cols);
    b->moves = 0;
}

/* Выделяет память под поле. Возвращает 0, если памяти не хватило. */
static int board_create(Board *b, int rows, int cols, int win)
{
    b->rows = rows;
    b->cols = cols;
    b->win = win;
    b->cells = (char *)malloc((size_t)rows * cols);
    if (b->cells == NULL)
        return 0;
    board_clear(b);
    return 1;
}

/* Рисует поле с номерами строк и столбцов. */
static void print_board(const Board *b)
{
    printf("\n    ");
    for (int c = 0; c < b->cols; c++)
        printf(" %-3d", c + 1);                 /* номера столбцов */
    printf("\n");

    for (int r = 0; r < b->rows; r++) {
        printf("   +");                         /* горизонтальная линия */
        for (int c = 0; c < b->cols; c++)
            printf("---+");
        printf("\n%2d |", r + 1);               /* номер строки */
        for (int c = 0; c < b->cols; c++)
            printf(" %c |", at(b, r, c));
        printf("\n");
    }
    printf("   +");
    for (int c = 0; c < b->cols; c++)
        printf("---+");
    printf("\n\n");
}

/* ---------- правила ---------- */

/* Проверяет, выиграл ли последний ход в клетке (r, c). */
static int check_win(const Board *b, int r, int c)
{
    /* четыре направления: горизонталь, вертикаль, две диагонали */
    static const int dir[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
    char sym = at(b, r, c);

    for (int d = 0; d < 4; d++) {
        int count = 1;                          /* сама клетка хода */
        for (int sign = -1; sign <= 1; sign += 2) {     /* в обе стороны */
            int y = r + sign * dir[d][0];
            int x = c + sign * dir[d][1];
            while (y >= 0 && y < b->rows && x >= 0 && x < b->cols && at(b, y, x) == sym) {
                count++;
                y += sign * dir[d][0];
                x += sign * dir[d][1];
            }
        }
        if (count >= b->win)
            return 1;
    }
    return 0;
}

/* ---------- ходы ---------- */

/* Ход человека. Возвращает 0, если ввод закончился. */
static int human_move(const Board *b, char sym, int *r, int *c)
{
    while (!eof_reached) {
        printf("Ходит %c. Введите строку и столбец через пробел: ", sym);
        if (!read_numbers(2, r, c)) {
            if (!eof_reached)
                printf("Ошибка: нужно два числа, например: 2 3\n");
        } else if (*r < 1 || *r > b->rows || *c < 1 || *c > b->cols) {
            printf("Ошибка: строка от 1 до %d, столбец от 1 до %d.\n", b->rows, b->cols);
        } else if (at(b, *r - 1, *c - 1) != EMPTY) {
            printf("Ошибка: эта клетка уже занята.\n");
        } else {
            (*r)--;                             /* переходим к нумерации с 0 */
            (*c)--;
            return 1;
        }
    }
    return 0;
}

/* Ход бота: случайная свободная клетка. */
static void bot_move(const Board *b, int *r, int *c)
{
    int total = b->rows * b->cols;
    int pick = rand() % (total - b->moves);     /* какая по счёту свободная */

    for (int i = 0; i < total; i++) {
        if (b->cells[i] == EMPTY && pick-- == 0) {
            *r = i / b->cols;
            *c = i % b->cols;
            return;
        }
    }
}

/* партия */

static void play_round(Board *b, int vs_bot)
{
    char who = 'X';                             /* крестики ходят первыми */

    board_clear(b);
    while (!eof_reached) {
        int r, c;

        print_board(b);
        if (vs_bot && who == 'O') {
            bot_move(b, &r, &c);
            printf("Бот ходит: строка %d, столбец %d\n", r + 1, c + 1);
        } else if (!human_move(b, who, &r, &c)) {
            return;
        }
        b->cells[r * b->cols + c] = who;
        b->moves++;

        if (check_win(b, r, c)) {
            print_board(b);
            printf("Победили %c!\n", who);
            return;
        }
        if (b->moves == b->rows * b->cols) {
            print_board(b);
            printf("Ничья!\n");
            return;
        }
        who = (who == 'X') ? 'O' : 'X';
    }
}

/* Настройка поля и повтор партий. */
static void play_game(int vs_bot)
{
    Board b;
    int rows = ask_number("Количество строк", 3, MAX_SIZE);
    int cols = ask_number("Количество столбцов", 3, MAX_SIZE);
    int max_win = rows < cols ? rows : cols;
    int win = ask_number("Сколько в ряд для победы", 3, max_win);

    if (eof_reached)
        return;
    if (!board_create(&b, rows, cols, win)) {
        printf("Ошибка: не хватает памяти.\n");
        return;
    }
    do {
        play_round(&b, vs_bot);
    } while (!eof_reached && ask_number("Сыграть ещё раз? 1 - да, 0 - нет", 0, 1) == 1);

    free(b.cells);                              /* освобождаем память */
}

int main(void)
{
    srand((unsigned)time(NULL));                /* чтобы бот ходил по-разному */

    while (!eof_reached) {
        printf("\n=== КРЕСТИКИ-НОЛИКИ ===\n");
        printf("1. Играть против бота\n2. Играть против человека\n3. Выйти\n");
        int choice = ask_number("Ваш выбор", 1, 3);
        if (choice == 0 || choice == 3)
            break;
        play_game(choice == 1);
    }
    printf("До свидания!\n");
    return 0;
}
