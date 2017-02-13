#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int n;
char *file1;
char *file2;

int *from_child, *to_child, *from_parent, *to_parent;

// Вспомогательная функция, которая создаёт неименованный канал
// (pipe) и записывает в W дескриптор на запись, а в R дескриптор на чтение 
void make_pipe(int* R, int* W) {
	int fd[2];
	pipe(fd);
	(*R) = fd[0];
	(*W) = fd[1];
}

// Функция преобразования строки в шестнадатеричной СС
// в беззнаковое целое число
unsigned int from_hex(char* s) {
	unsigned int result = 0;
	// Проходим по строке слева направо
	for (; *s != '\0'; s++) {
		// Переводим символ в число
		unsigned int c;
		if ((*s) >= '0' && (*s) <= '9')
			c = (*s) - '0';
		else if ((*s) >= 'A' && (*s) <= 'F')
			c = (*s) - 'A' + 10;
		else
			c = (*s) - 'a' + 10;
		result = result * 16 + c;
	}
	return result;
}

int main(int argc, char** argv) {
	int i, id, fd2;
	unsigned int num;

	n = atoi(argv[1]);
	file1 = argv[2];
	file2 = argv[3];

	// Перед тем, как создать процесс мы заведем очень много pipe
	// для того, чтобы процессы могли общаться

	// У нас будет 4(!) массива:
	//
	//    1. from_child[i] содержит файловый дескриптор, через который
	//       отец будет слушать сообщения от i-го сына.
	//    2. from_parent[i] содержит файловый дескриптор, через который
	//       i-ый сын будет слушать отца.
	//    3. to_parent[i] содежит дескриптор, через который i-ый сын
	//       будет писать отцу
	//    4. to_child[i]. Ну, я думаю, и так понятно :)
	from_child = (int*)malloc(sizeof(int) * n);
	to_child = (int*)malloc(sizeof(int) * n);
	from_parent = (int*)malloc(sizeof(int) * n);
	to_parent = (int*)malloc(sizeof(int) * n);

	for (i = 0; i < n; ++i) {
		make_pipe(&from_child[i], &to_parent[i]);
		make_pipe(&from_parent[i], &to_child[i]);
	}

	// Протокол работы процессов будет такой:
	//   Как только отец хочет, чтобы сын считал число, он отправляет
	//   любой символ сыну. В моём решении это будет просто буква A.
	//
	//   Как только сын закончил это делать, он отправляет отцу одну букву.

	// Сразу откроем FILE2
	fd2 = open(file2, O_RDONLY);

	// Начнём запускать процессы в цикле
	for (id = 0; id < n; ++id)
		if (fork() == 0) {
			unsigned int result = 0;

			// === Если мы попали сюда, то мы id-ый сын. ===
			// Закрываем все pipe, которые относятся к отцу и другим сыновьям
			for (i = 0; i < n; ++i) {
				close(from_child[i]);
				close(to_child[i]);
				if (i != id) {
					close(from_parent[i]);
					close(to_parent[i]);
				}
			}

			// Пока нам что-то отправляют
			while (read(from_parent[id], &num, sizeof(char)) > 0) {
				// Считываем наше 8-значное число и перевод строки
				char s[9];
				read(fd2, s, sizeof(s));
				// Заменяем перевод строки на символ окончания строчки
				s[8] = '\0';
				// Получаем число из 16-ричной системы счисления в unsigned int
				num = from_hex(s);
				// Делаем сдвиг на id разрядов влево
				// Выпавшие разряды переносятся вправо и приписываются к числу
				// (они равны (num >> (32 - id)))
				num = (num << id) | (num >> (32 - id));
				// Делаем сложение по модулю 2 с result
				result = result ^ num;
				// Отправляем отцу через pipe, что мы всё сделали
				write(to_parent[id], "A", sizeof(char));
			}

			// Отец закрыл с нами соединение -> выводим ответ
			printf("%u\n", result);
			return 0;
			// === Конец кода id-го сына ===
		}

        // === Ок, если мы тут, то мы отец. ===
	// Закрываем все pipe, которые относятся к сыновьям
	for (i = 0; i < n; ++i) {
		close(from_parent[i]);
		close(to_parent[i]);
	}
	// FILE2 в отце нам тоже не нужен
	close(fd2);
	
	// Откроем FILE1 и сразу перенаправим его в стандартный ввод с помощью dup2
	// Это позволит нам считывать данные из файла обычным scanf
	dup2(open(file1, O_RDONLY), 0);

	// Пока есть числа в FILE1, считываем. %u значит, что считываем unsigned int
	while (scanf("%u", &num) != EOF) {
		// После считывания числа num нужно обратиться к сыну под
		// номером num % n
		write(to_child[num % n], "A", sizeof(char));
		// Ждем ответа сына
		read(from_child[num % n], &num, sizeof(char));
	}

	// Ок, FILE1 кончился, нужно убить детей :)
	for (id = 0; id < n; ++id) {
		close(to_child[id]);
		close(from_child[id]);
		wait(NULL);
	}
	printf("0\n");
	return 0;
}
