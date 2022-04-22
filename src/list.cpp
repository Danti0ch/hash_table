#include "list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

const int INCREASE_RATIO = 1;
const int REDUCE_RATIO   = 3;

// режим для resize
const int INCREASE_MODE  = 1;
const int REDUCE_MODE    = 0;

//========================================================================================//

//

//                          LOCAL_FUNCTIONS_DECLARATION

//

//========================================================================================//

#define list_resize(obj, mode) _list_resize((obj), (mode), #obj, LOCATION)

#define  extract_free_ind(obj) _extract_free_ind((obj), #obj, LOCATION)

LIST_ERR_CODE _list_resize(list* obj, int mode, META_PARAMS);

int _extract_free_ind(list* obj, META_PARAMS);

int node_cmp(const void * node1, const void * node2);

LIST_VERIF_CODE list_verification(const list* obj);

void print_list(const list* obj);

int get_file_size(const char* name);

#if ENABLE_SORT
void list_sort(list* obj);
#endif




// 								WOWOWW
//
//
///
extern "C" int fstrcmp(const char* str1, const char* str2);
//
//
//
//								WWOWOWOWO






/**
 * @brief ищет физический индекс по логическому
 * 
 * @param obj указатель на обьект списка
 * @param pos логический индекс узла
 * @return физический индекс узла 
 */
int search_phys_pos(list* obj, int pos);

// checker that node is in list
inline int node_in_list(const list* obj, const node* nod){
	if(nod >= obj->nodes + obj->capacity || nod < obj->nodes){
		return 0;
	}
	return 1;
}

// вставляем в любом случае за единицу
// в любом случае реализую is_sorted, сортировку, сверхбыструю адресацию по логическим адресам
// так как поиск энивей за O(n), будем сортить предподсчётом за O(n), нужно прописать в доку, что юзается много памяти

// TODO: fix front.prev = -1
// TODO: может быть ошибка при вставке на позицию size
// TODO: слишком много ифов в методах, нужна оптимизация
// TODO: более продуманые ресайзы
// TODO: аккуратно проставить is_sorted = UNSORTED в методах вставки/удаления
// TODO: сделать отдельный файл под верификацию??
// TODO: дамп в графвизе
// TODO: юнит тесты

// ну да, поиск энивей за O(n), так что просто посортим за O(n). Сделаю возможность отключить сортировку через макрос

//========================================================================================//

//

//                          PUBLIC_FUNCTIONS_DEFINITION

//

//========================================================================================//

list* _ListConstructor(const size_t capacity, LOC_PARAMS){

	if(capacity == 0) return NULL;

	list* obj = (list*)calloc(1, sizeof(list));

	obj->nodes = (node*)calloc(capacity, sizeof(node));

/*
	if(obj->nodes == NULL){
		return LIST_ERR_CODE::MEM_ALLOC;
	}
*/

	// создаем односвязный список свободных элементов
	// индикатор незанятости элемента - FREE_MATE в prev
	for(int n_node = 0; n_node < capacity; n_node++){
		obj->nodes[n_node].val  = POISON_VAL;
		obj->nodes[n_node].next = n_node + 1;
		obj->nodes[n_node].prev = FREE_MATE;
	}

	obj->nodes[capacity - 1].next = FREE_MATE;

#if ENABLE_SORT
	obj->is_sorted 	= SORTED;
#endif

	obj->head 		= FREE_MATE;
	obj->tail 		= FREE_MATE;

	obj->head_free 	= 0;
	obj->tail_free 	= capacity - 1;

	obj->size 		= 0;
	obj->capacity 	= capacity;

	char obj_name[] = "costil";
	LIST_OK(obj)

	return obj;
}
//----------------------------------------------------------------------------------------//

// OK
LIST_ERR_CODE _ListDestructor(list* obj, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	free(obj->nodes);
	free(obj);


/*
	obj->nodes = (node*)FREE_MEM;	
	obj->head 		= FREE_MATE;
	obj->tail 		= FREE_MATE;
	obj->head_free  = FREE_MATE;
	obj->tail_free  = FREE_MATE;
	obj->capacity 	= 0;
	obj->size 		= 0;

#if ENABLE_SORT
	obj->is_sorted  = 1;
#endif
*/
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK
node* _ListFront(const list* obj, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	if(obj->size > 0) return obj->nodes + obj->head;
	else return NULL;
}
//----------------------------------------------------------------------------------------//

// OK
node* _ListBack(const list* obj, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	if(obj->size > 0) return obj->nodes + obj->tail;
	else return NULL;
}
//----------------------------------------------------------------------------------------//

// OK
node* _ListAfter(const list* obj, const node* nod, META_PARAMS){

	assert(obj  != NULL);
	assert(nod  != NULL);
	LIST_OK(obj)

	if(!node_in_list(obj, nod)) return NULL;

	// после любого элемента кроме хвоста есть следующий элемент
	if(nod != obj->nodes + obj->tail){
		return obj->nodes + nod->next;
	}
	else return NULL;
}
//----------------------------------------------------------------------------------------//

// OK
node* _ListBefore(const list* obj, const node* nod, META_PARAMS){

	assert(obj  != NULL);
	assert(nod  != NULL);
	LIST_OK(obj)

	if(!node_in_list(obj, nod)) return NULL;

	// после любого элемента кроме головы есть предыдущий элемент	
	if(nod != obj->nodes + obj->head){
		return obj->nodes + nod->prev;
	}
	else return NULL;
}
//----------------------------------------------------------------------------------------//

// OK

// TODO: если я захочу, чтобы массив всегда был отсортирован - тогда тут лучше сделать специальную функцию для сортировки,
// а в pushback'e просто в конец массива вставлять
LIST_ERR_CODE _PushFront(list* obj, const list_T val, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	if(obj->size >= obj->capacity){
		int resize_rez = (int)list_resize(obj, INCREASE_MODE);
		
		if(resize_rez != (int)LIST_ERR_CODE::OK){
			return (LIST_ERR_CODE)resize_rez;
		}
	}

	int new_head_pos = extract_free_ind(obj);

	obj->nodes[new_head_pos].next = obj->head;
	obj->nodes[new_head_pos].prev = FREE_MATE;

	if(obj->head != FREE_MATE){
		obj->nodes[obj->head].prev = new_head_pos;
	}

	obj->head = new_head_pos;
	obj->nodes[obj->head].val = val;

	if(obj->size == 0){
		obj->tail = new_head_pos;
	}

	obj->size++;

	// может как-то попродуманее?
#if ENABLE_SORT
	obj->is_sorted = UNSORTED;
#endif
	LIST_OK(obj)
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK

LIST_ERR_CODE _PushBack(list* obj, const list_T val, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	if(obj->size >= obj->capacity){
		int resize_rez = (int)list_resize(obj, INCREASE_MODE);
		
		if(resize_rez != (int)LIST_ERR_CODE::OK){
			return (LIST_ERR_CODE)resize_rez;
		}
	}

	int new_tail_pos = extract_free_ind(obj);

	obj->nodes[new_tail_pos].next = FREE_MATE;
	obj->nodes[new_tail_pos].prev = obj->tail;

	if(obj->tail != FREE_MATE){
		obj->nodes[obj->tail].next = new_tail_pos;
	}

	obj->tail = new_tail_pos;
	obj->nodes[obj->tail].val = val;

	if(obj->size == 0){
		obj->head = new_tail_pos;
	}
	obj->size++;

	LIST_OK(obj)

	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK
// да! будем сортить сразу после вставки/удаления. Тогда операция вставки будет за O(n * const) (radix), или за O(n) (count_sort)
LIST_ERR_CODE _ListInsert(list* obj, const uint pos, const list_T val, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj);

	// если = obj->size, то вставляем в конец как pushback'e
	if(pos < 0 || pos > obj->size){
		return LIST_ERR_CODE::INVALID_POS;
	}

	if(obj->size >= obj->capacity){
		int resize_rez = (int)list_resize(obj, INCREASE_MODE);
		
		if(resize_rez != (int)LIST_ERR_CODE::OK){
			return (LIST_ERR_CODE)resize_rez;
		}
	}

	// в search_phys_pos должна вернуть сделать массив отсортированным, если он уже не отсортирован
	int pos_of_prev = search_phys_pos(obj, pos);

	int new_node_pos = extract_free_ind(obj);

	obj->nodes[new_node_pos].next = obj->nodes[pos_of_prev].next;
	obj->nodes[new_node_pos].prev = pos_of_prev;
	obj->nodes[new_node_pos].val  = val;

	obj->nodes[pos_of_prev].next 					= new_node_pos;
	obj->nodes[obj->nodes[new_node_pos].next].prev 	= new_node_pos;

#if ENABLE_SORT
	// если вставка не в конец, то логич порядок != физическому
	if(pos != obj->size){
		obj->is_sorted = UNSORTED;
	}
#endif
	obj->size++;

	LIST_OK(obj)
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK
LIST_ERR_CODE _ListRemove(list* obj, const int pos, META_PARAMS){

	assert(obj != NULL);

	LIST_OK(obj);

	if(pos < 0 || pos >= obj->size){
		return LIST_ERR_CODE::INVALID_POS;
	}

	if((obj->capacity >> REDUCE_RATIO) >= obj->size){
		int resize_rez = (int)list_resize(obj, REDUCE_MODE);

		if(resize_rez != (int)LIST_ERR_CODE::OK){
			return (LIST_ERR_CODE)resize_rez;
		}
	}

	int physical_pos = search_phys_pos(obj, pos);

	int pos_next = obj->nodes[physical_pos].next;
	int pos_prev = obj->nodes[physical_pos].prev;

	// не уверен, нужен тест
	if(pos_prev != FREE_MATE){
		obj->nodes[pos_prev].next = pos_next;
	}
	else{
		obj->head = pos_next;
	}

	if(pos_next != FREE_MATE){
		obj->nodes[pos_next].prev = pos_prev;
	}
	else{
		obj->tail = pos_prev;
	}

	if(obj->tail_free != FREE_MATE){
		obj->nodes[obj->tail_free].next = physical_pos;
		obj->nodes[physical_pos].prev   = obj->tail_free;
	}
	else{
		obj->head_free                = physical_pos;
		obj->nodes[physical_pos].prev = FREE_MATE;
	}

	obj->tail_free                = physical_pos;
	obj->nodes[physical_pos].val  = POISON_VAL;
	obj->nodes[physical_pos].next = FREE_MATE;

#if ENABLE_SORT
	if(pos != obj->size){
		obj->is_sorted = UNSORTED;
	}
#endif

	obj->size--;

	LIST_OK(obj)
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK
LIST_ERR_CODE _ListRemoveAll(list* obj, META_PARAMS){

	assert(obj != NULL);
	LIST_OK(obj)

	for(int n_node = 0; n_node < obj->capacity; n_node++){
		obj->nodes[n_node].val  = POISON_VAL;
		obj->nodes[n_node].next = n_node + 1;
		obj->nodes[n_node].prev = FREE_MATE;
	}

	obj->nodes[obj->capacity - 1].next = POINTER_TO_VOID;

	obj->size = 0;
	obj->head_free = 0;
	obj->tail_free = obj->capacity - 1;

	obj->head = FREE_MATE;
	obj->tail = FREE_MATE;

#if ENABLE_SORT
	obj->is_sorted = SORTED;
#endif
	LIST_OK(obj)
	
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//


/*

	uint size = obj->size;
	int temp = 0;
	for(uint n_node = 0; n_node < size; n_node++){
		printf("%u\n", n_node);
		asm(".intel_syntax noprefix\n\t" // директива GAS, включаем Intel синтаксис.
			"mov rdi, %1\n\t"            // прибавляем значение переменной b к eax.
			"mov rsi, %2\n\t"            // перемещаем в переменную c значение eax.
			".att_syntax prefix\n\t"
			"call fstrcmp\n\t"
			".intel_syntax noprefix\n\t"
			"mov %0, eax\n\t"
			".att_syntax prefix\n\t"
			:"=r"(temp)
			:"r"(cur_node[n_node].val), "r"(val), "r"(cur_node + n_node)
			: "rdi", "rsi", "rax",  "rdx", "rbx"                     // список разрушаемых регистров.
			);
		asm(".att_syntax prefix\n\t");
		if(temp == 0){
			return cur_node + n_node;
		}
	}
*/

node* _ListFind(const list* obj, const list_T val, META_PARAMS){
	
	LIST_OK(obj)

	if(obj->size == 0) return NULL;

	node* cur_node = obj->nodes + obj->head;

	// TODO: sort + strcmp on asm to optimize

#if ENABLE_SORT

	uint size = obj->size;

	for(uint n_node = 0; n_node < size; n_node++){
		if(fstrcmp(cur_node[n_node].val, val) == 0) return cur_node + n_node;
	}
#else
	for(uint n_node = 0; n_node < obj->size; n_node++, cur_node = obj->nodes + cur_node->next){
		if(strcmp(cur_node->val, val) == 0) return cur_node;
	}
#endif // ENABLE_SORT
	return NULL;
}
//----------------------------------------------------------------------------------------//


LIST_VERIF_CODE VerifyList(const list* obj){

	assert(obj != NULL);

	if(obj->nodes == NULL){
		return LIST_VERIF_CODE::CORRUPTED_MEM;
	}

	if(obj->head != FREE_MATE && obj->head >= obj->capacity){
		return LIST_VERIF_CODE::HEAD_INVALID;
	}
	
	if(obj->capacity < obj->size){
		return LIST_VERIF_CODE::CORRUPTED_SIZE;
	}

	#if DEBUG_MODE == LINEAR_CHECKS

	uint8_t* visited = (uint8_t*)calloc(obj->capacity, sizeof(uint8_t));

	// TODO: free(visited) make syntax sugar
	if(obj->size > 0){

		// checking data part
		if(obj->head == FREE_MATE || (obj->head >= obj->capacity || obj->head < 0)){
			free(visited);
			return LIST_VERIF_CODE::HEAD_INVALID;
		}

		int n_node = obj->head;
		int next_ind = obj->nodes[n_node].next;
		
		for(; next_ind != FREE_MATE; visited[n_node] = 1, n_node = next_ind, next_ind = obj->nodes[next_ind].next){

			if(visited[n_node]){
				free(visited);
				return LIST_VERIF_CODE::CORRUPTED_INJECTIVITY;
			}

			// TODO: optimize ?
			if(n_node != obj->head && obj->nodes[n_node].prev == FREE_MATE){
				free(visited);
				return LIST_VERIF_CODE::PREV_CORRUPTED;
			}

			if(next_ind >= obj->capacity || next_ind < 0){
				free(visited);
				return LIST_VERIF_CODE::CORRUPTED_LINK;
			}

			if(n_node != obj->nodes[next_ind].prev){
				free(visited);
				return LIST_VERIF_CODE::CORRUPTED_LINK;
			}
		}

		if(n_node != obj->tail){
			free(visited);
			return LIST_VERIF_CODE::TAIL_INVALID;
		}
		visited[n_node] = 1;
	}

	// checking free items sublist

	if(obj->capacity != obj->size){

		if(obj->head_free == FREE_MATE || (obj->head_free < 0 || obj->head_free >= obj->capacity)){
			free(visited);
			return LIST_VERIF_CODE::HEAD_FREE_CORRUPTED;
		}

		int n_node = obj->head_free;
		int next_ind = obj->nodes[n_node].next;

		for(; next_ind != FREE_MATE; visited[n_node] = 1, n_node = next_ind, next_ind = obj->nodes[next_ind].next){

			if(obj->nodes[n_node].prev != FREE_MATE){
				free(visited);
				return LIST_VERIF_CODE::FREE_CORRUPTED;
			}
			
			if(visited[n_node]){
				free(visited);
				return LIST_VERIF_CODE::CORRUPTED_INJECTIVITY;
			}

			if(next_ind >= obj->capacity || next_ind < 0){
				free(visited);
				return LIST_VERIF_CODE::CORRUPTED_LINK;
			}
		}

		if(obj->tail_free != n_node){
			free(visited);
			return LIST_VERIF_CODE::TAIL_FREE_CORRUPTED;
		}

		visited[n_node] = 1;
	}

	// checking that all nodes were covered
	for(int n_node = 0; n_node < obj->capacity; n_node++){

		if(!visited[n_node]){
			free(visited);
			return LIST_VERIF_CODE::VOID_ELEMENT;
		}
	}

	free(visited);
	#endif // DEBUG_MODE == LINEAR_CHECKS

	return LIST_VERIF_CODE::OK;
}
//----------------------------------------------------------------------------------------//

void ListDump(const list* obj, meta_info *meta, META_PARAMS){

	assert(obj  != NULL);
	assert(meta != NULL);

#if ENABLE_SORT

	LOG("<br><br>Dump called from function %s(%d)<br>"
		   "That was called from file %s, function %s(%d)<br>"
		   "[#] list %s<int><br>"
		   "<br>"
		   "capacity  = %lu<br>"
		   "size      = %lu<br>"
		   "head      = %d<br>"
		   "tail 	  = %d<br>"
		   "head_free = %d<br>"
		   "tail_free = %d<br>"
		   "is_sorted = %u<br>"
		   "<br>",
		   func_name, n_line, meta->file_name, meta->func_name, meta->n_line,
		   meta->obj_name,
		   obj->capacity, obj->size, obj->head, obj->tail, obj->head_free, obj->tail_free, obj->is_sorted);

#else
		LOG("<br><br>Dump called from function %s(%d)<br>"
		   "That was called from file %s, function %s(%d)<br>"
		   "[#] list %s<int><br>"
		   "<br>"
		   "capacity  = %lu<br>"
		   "size      = %lu<br>"
		   "head      = %d<br>"
		   "tail 	  = %d<br>"
		   "head_free = %d<br>"
		   "tail_free = %d<br>"
		   "<br>",
		   func_name, n_line, meta->file_name, meta->func_name, meta->n_line,
		   meta->obj_name,
		   obj->capacity, obj->size, obj->head, obj->tail, obj->head_free, obj->tail_free);

#endif // ENABLE_SORT

	DumpNodes(obj);

	LOG("<br><br>");
	//print_list(obj);
	//to_log("<br><br>");
	
	return;
}
//----------------------------------------------------------------------------------------//


// TODO: для пользовательского дампа добавить аргумент - функцию вывода нод.val
// OK
void DumpNodes(const list* obj){
	
	assert(obj != NULL);

	LOG("nodes = [");
	for(int n_node = 0; n_node < obj->capacity; n_node++){
		LOG("(%s, %d, %d), ", obj->nodes[n_node].val, obj->nodes[n_node].next, obj->nodes[n_node].prev);
	}
	LOG("]<br>");

	return;
}

//========================================================================================//

//

//                          LOCAL_FUNCTIONS_DEFINITION

//

//========================================================================================//

// OK
int search_phys_pos(list* obj, int pos){
	
	assert(obj != NULL);
	//LIST_OK(obj);

	// проверка на корректность pos
	if(pos < 0 || pos >= obj->size) return FREE_MATE;

#if ENABLE_SORT
	// если is_sorted, то логический pos сооотв. физическому
	if(obj->is_sorted == UNSORTED){
		list_sort(obj);
	}
	return pos;
#else
	int counter = 0;

	int n_node = obj->head;

	while((counter < pos) && (n_node != FREE_MATE)){
		counter++;
		n_node = obj->nodes[n_node].next;
	}

	return n_node;
#endif


}
//----------------------------------------------------------------------------------------//

// OK
int _extract_free_ind(list* obj, META_PARAMS){

	assert(obj != NULL);
	//LIST_OK(obj);

	int ind = obj->head_free;

	if(obj->nodes[ind].next != FREE_MATE){
		int head_free_next = obj->nodes[ind].next;
		obj->head_free = head_free_next;
	}
	else{
		obj->head_free = FREE_MATE;
		obj->tail_free = FREE_MATE;
	}

	return ind;
}
//----------------------------------------------------------------------------------------//

// OK
LIST_ERR_CODE _list_resize(list* obj, int mode, META_PARAMS){

	assert(obj != NULL);
	//LIST_OK(obj)

	if(mode == INCREASE_MODE){

		int new_capacity = obj->capacity << INCREASE_RATIO;
		
		node* p_new_mem = (node*)calloc(new_capacity, sizeof(node));

		if(p_new_mem == NULL){
			return LIST_ERR_CODE::MEM_ALLOC;
		}

		for(int n_node = 0; n_node < obj->capacity; n_node++){
			p_new_mem[n_node].next  = n_node + 1;
			p_new_mem[n_node].prev  = n_node - 1;
			p_new_mem[n_node].val   = obj->nodes[n_node].val;
		}

		p_new_mem[0].prev                 = FREE_MATE;
		p_new_mem[obj->capacity - 1].next = FREE_MATE;

		for(int n_node = obj->capacity; n_node < new_capacity ; n_node++){
			p_new_mem[n_node].next = n_node + 1;
			p_new_mem[n_node].prev = FREE_MATE;
		}
		
		p_new_mem[new_capacity - 1].next = FREE_MATE;

		if(obj->tail_free != FREE_MATE){
			obj->nodes[obj->tail_free].next = obj->capacity;
		}
		else{
			obj->head_free = obj->capacity;
		}
		obj->tail_free = new_capacity - 1;

		free(obj->nodes);
		obj->nodes    = p_new_mem;
		obj->capacity = new_capacity;
	}

// TODO: тут явно могут быть проблемы, раскоменчу, когда все остальное заработает
	/*
	else if(mode == REDUCE_MODE){
		
		SortList(obj);

		int new_capacity = obj->capacity / REDUCE_RATIO;
		
		node* p_new_mem = (node*)calloc(new_capacity, sizeof(node));
	
		if(p_new_mem == NULL){
			return LIST_ERR_CODE::MEM_ALLOC;
		}

		memcpy(p_new_mem, obj->nodes, new_capacity * sizeof(node));

		free(obj->nodes);

		obj->nodes = p_new_mem;
		obj->capacity = new_capacity;
		p_new_mem = NULL;

		if(obj->nodes[new_capacity - 1].prev == FREE_INDICATION_VAL){
			obj->nodes[new_capacity - 1].next = 0;
		}
	}
	*/
	//LIST_OK(obj)
	return LIST_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// OK
// TODO: обязательно протестить
int node_cmp(const void * node1, const void * node2){

	assert(node1 != NULL);
	assert(node2 != NULL);
	assert(node1 != node2);

	// конвертим в uint, чтобы -1(FREE_MATE) было максимальным значением 
	return (int)((uint32_t)(((node*)node1)->prev)) - (int)((uint32_t)(((node*)node2)->prev));
}
//----------------------------------------------------------------------------------------//

// OK
// TODO: оптимизация - xor swap
void nodes_swap(node* a, node* b){
	
	assert(a != NULL);
	assert(b != NULL);
	assert(a != b);

	node c;

	memcpy(&c, a, sizeof(node));
	memcpy(a, b, sizeof(node));
	memcpy(b, &c, sizeof(node));	

	return;
}
//----------------------------------------------------------------------------------------//

#if ENABLE_SORT

// OK
// TODO: обработка in-place сортировки, когда не можем вылелить вспомогательный массив
void list_sort(list* obj){

	assert(obj != NULL);

	node* swap_arr = (node*)calloc(obj->capacity, sizeof(node));

	if(obj->size != 0){
		int cur_n_node = obj->head;

		for(int n_swap_node = 0; n_swap_node < obj->size; n_swap_node++, cur_n_node = obj->nodes[cur_n_node].next){
			swap_arr[n_swap_node].val  = obj->nodes[cur_n_node].val;
			swap_arr[n_swap_node].next = n_swap_node + 1;
			swap_arr[n_swap_node].prev = n_swap_node - 1;
		}
		swap_arr[0].prev             = FREE_MATE;
		swap_arr[obj->size - 1].next = FREE_MATE;
	}

	if(obj->size != obj->capacity){
		int cur_n_node = obj->head_free;

		for(int n_swap_node = obj->size; n_swap_node < obj->capacity; n_swap_node++, cur_n_node = obj->nodes[cur_n_node].next){
			swap_arr[n_swap_node].val  = POISON_VAL;
			swap_arr[n_swap_node].next = n_swap_node + 1;
			swap_arr[n_swap_node].prev = FREE_MATE;
		}
		swap_arr[obj->capacity - 1].next = FREE_MATE;
	}

	free(obj->nodes);
	obj->nodes = swap_arr;
	obj->is_sorted = SORTED;

	return;
}
//----------------------------------------------------------------------------------------//
#endif //ENABLE_SORT

//----------------------------------------------------------------------------------------//

/*
void print_list(list* obj){

	assert(obj != NULL);

	FILE* tmp_file = fopen(GVIZ_DOT_NAME, "w");

	fprintf(tmp_file, "digraph structs {\n"
			          "rankdir=LR;\n");

	if(obj->capacity > 0){
		fprintf(tmp_file, "0 [style=\"filled\", fillcolor=\"#BB8FCE\", shape=record,label=\"  <main0> 0 | %d | { %d | %d }\"];\n", 
			    obj->nodes[0].val, obj->nodes[0].prev, obj->nodes[0].next);
	}
	for(int n_node = 1; n_node < obj->capacity; n_node++){

		fprintf(tmp_file, "%d [style=\"filled\", fillcolor=\"%s\", shape=record,label=\"  <main%d> %d | %d | { %d | %d }\"];\n", 
			    n_node, (obj->nodes[n_node].prev == FREE_INDICATION_VAL) ? "#AED6F1" : "#82E0AA",
			    n_node, n_node, obj->nodes[n_node].val, obj->nodes[n_node].prev, obj->nodes[n_node].next);
	}

	fprintf(tmp_file, "{\n"
    				  "node[pin=true];\n"
    				  "edge[style=invis];\n");

	for(int n_node = 0; n_node < obj->capacity - 1; n_node++){
		fprintf(tmp_file, "%d->", n_node);
	}

	fprintf(tmp_file, "%d;\n}\n\n", obj->capacity - 1);

	for(int n_node = obj->head; obj->nodes[n_node].next != 0; n_node = obj->nodes[n_node].next){
		fprintf(tmp_file, "%d->%d;\n", n_node, obj->nodes[n_node].next);
	}

	for(int n_node = obj->head_free; obj->nodes[n_node].next != 0; n_node = obj->nodes[n_node].next){
		fprintf(tmp_file, "%d->%d[color=\"blue\"];\n", n_node, obj->nodes[n_node].next);
	}

	fprintf(tmp_file, "\n}\n");
	fclose(tmp_file);

	char* cmd_str = (char*)calloc(CMD_STR_SIZE, sizeof(char));

	// TODO: придумать как исправить этот костыль
	sprintf(cmd_str, "dot -Tsvg %s -o %s", GVIZ_DOT_NAME, GVIZ_HTM_NAME);
	WinExec(cmd_str, 0);
	Sleep(80);
	// КОСТЫЛЬ

    int buflen = get_file_size(GVIZ_HTM_NAME);

	tmp_file = fopen(GVIZ_HTM_NAME, "r");
 	
	char buffer[buflen];

	fread(buffer, sizeof(char), buflen, tmp_file);

	fclose(tmp_file);
	to_log(buffer);

	return;
}

int get_file_size(const char* name){
	
	assert(name != NULL);

	struct stat st;
    stat(name, &st);

    return st.st_size;	
}
*/
