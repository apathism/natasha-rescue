#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// В отличие от предыдущих задач у нас ещё есть prev кроме next
// Это скорее усложняет дело, так как prev не очень нужен, но его нужно
// обновлять
struct Elem {
	struct Elem *next, *prev;
	int count;
	char *str;
};

// Будем решать задачу рекурсивно
struct Elem *process (struct Elem *head) {
	int i;
	struct Elem *x;

	// Если список пуст, то есть head == NULL,
	// то можно просто завершить функцию
	if (!head) return head;

	// Если у элемента count == 1, то почти ничего
	// делать не нужно, только рекурсивно запуститься
	// от next
	process(head->next);
	if (head->count == 1)
		return head;

	// Если у нас есть элемент, у которого count > 1,
	// то нам нужно создать массив, в котором будет ещё count - 1
	// элемент. Этот массив мы создадим с помощью malloc
	x = (struct Elem*)malloc(sizeof(struct Elem) * (head->count-1));
	
	// Мы должны пройтись по массиву (по элементам x[0], x[1], ...)
	// и в каждом элементе заполнить count = 1, а str строкой, описанной
	// в условии задачи. Кроме того, должны быть заполнены указатели
	// x[i].next = &x[i + 1]
	// x[i].prev = &x[i - 1]
	for (i = 0; i < head->count - 1; ++i) {
		// number -- это номер k, который мы дописываем к строке
		int number = i + 2;

		x[i].count = 1;
		x[i].prev = head;
		x[i].next = head->next;

		// Если есть предыдущий элемент и следующий элемент,
		// то мы можем обновить указатель на них. Если нет,
		// то это соответственно последний или первый из созданных
		// элементов.
		if (number > 2) x[i].prev = &x[i - 1];
		if (number < head->count) x[i].next = &x[i + 1];

		// Для того, чтобы получить строчку вида b-2, ..., b-count
		// воспользуемся функция sprintf, которая делает то же самое, что
		// и printf, только выводит результат не на экран, а в строку, которая даётся
		// в первом параметре.
		
		// Создаём строку
		x[i].str = malloc((strlen(head->str) + 20) * sizeof(char));
		sprintf(x[i].str, "%s-%d", head->str, number);
	}
	// Отлично. Теперь у нас есть много элементов вида b-2, b-3, b-4, ..., b-count

	// Нужно указать для head->next (если он есть), что перед ним теперь идёт b-count
	if (head->next) head->next->prev = &x[head->count - 2];
	// Нужно указать, что у head следующий элемент -- b-2
	head->next = &x[0];
	// Нужно обновить count у head
	head->count = 1;

	// Вот и всё, ребята!
	return head;
}
