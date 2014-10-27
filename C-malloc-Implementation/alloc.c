#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef struct _tag{
	size_t size;
	char taken;
}tag;
#define N sizeof(tag)

typedef struct _payload{
	struct _payload* previous;
	struct _payload* next;
}payload;

void* start = NULL;
void* end = NULL;
payload* firstfree = NULL;
void* malloc(size_t size){
	void* return_ptr = NULL;
	if(start == NULL){
		start = sbrk(0);
		end = start;
	}
	if(size == 0){
		return NULL;
	}
	if(size < sizeof(payload)){
		size = sizeof(payload);
	}
payload* james = firstfree;

	if(firstfree){
			while(1){
			if(((tag*)((void*)james - N))->size >= size){
				if(james == firstfree){
					if(james->next == NULL){
						firstfree = NULL;
					}
					else{
						(james->next)->previous = NULL;
						firstfree = james->next;
					}
				}
				else if(james->next == NULL){
					(james->previous)->next = NULL;
				}
				else{
					(james->previous)->next = james->next;
					(james->next)->previous = james->previous;
				}

				if(((tag*)((void*)james - N))->size - size > 100000){
					void* head1 = (void*)james - N;
					void* tail1 = (void*)james + size;
					void* head2 = (void*)tail1 + N;
					void* tail2 = head1 + ((tag*)head1)->size + N;
					size_t size2 = ((tag*)head1)->size - size - 2*N;
					((tag*)head1)->size = size;
					((tag*)head1)->taken = 2;
					((tag*)tail1)->size = size;
					((tag*)tail1)->taken = 2;
					((tag*)head2)->size = size2;
					((tag*)tail2)->size = size2;
					free(head2 + N);
				}
				void* head = (void*)james - N;
				((tag*)head)->taken = 2;
				void* tail = (void*)james + ((tag*)head)->size;
				((tag*)tail)->taken = 2;
				return_ptr = (void*)james;
				return return_ptr;
			}
			if(james->next == NULL)break;
			else
				james = james->next;
		}
	}
	return_ptr = sbrk(size + 2*N);
	end = return_ptr + size + 2*N;
	((tag*)return_ptr)->size = size;
	((tag*)(return_ptr + size + N))->size = size;
	((tag*)(return_ptr))->taken = 2;
	((tag*)(return_ptr))->taken = 2;
	return_ptr = return_ptr + N;
	return return_ptr;
}

void free(void* ptr){
	if(ptr == NULL)
		return;
	payload* ben = (payload*)ptr;
	void* head = ptr - N;
	void* tail = ptr + ((tag*)head)->size;
	if(firstfree == NULL){
		firstfree = ben;
		((tag*)head)->taken = 1;
		((tag*)tail)->taken = 1;
		ben->next = NULL;
		ben->previous = NULL;
		return;
	}
	if(head == start){
		void* after = tail + N;
		payload* afterpayload = (payload*)(after + N);
		if(((tag*)after)->taken == 1){
			if(afterpayload == firstfree){
				if(afterpayload->next == NULL){
					firstfree = NULL;
				}
				else{
					(afterpayload->next)->previous = NULL;
					firstfree = afterpayload->next;
				}
			}
			else if(afterpayload->next == NULL){
				afterpayload->previous = NULL;
			}
			else{
				(afterpayload->previous)->next = afterpayload->next;
				(afterpayload->next)->previous = afterpayload->previous;
			}
			if(firstfree == NULL){
				firstfree = ben;
				ben->next = NULL;
				ben->previous = NULL;
   				size_t newsize = ((tag*)head)->size + ((tag*)after)->size + 2*N;
				((tag*)head)->size = newsize;
				((tag*)head)->taken = 1;
				((tag*)(after + ((tag*)after)->size + N))->size = newsize;
				((tag*)(after + ((tag*)after)->size + N))->taken = 1;

				return;

			}
			size_t newsize = ((tag*)head)->size + ((tag*)after)->size + 2*N;
			((tag*)head)->size = newsize;
			((tag*)head)->taken = 1;
			((tag*)(after + ((tag*)after)->size + N))->size = newsize;
			((tag*)(after + ((tag*)after)->size + N))->taken = 1;
			ben->next = firstfree;
			ben->previous = NULL;
			firstfree->previous = ben;
			firstfree = ben;
			return;
		}
		else{
			((tag*)head)->taken = 1;
			((tag*)tail)->taken = 1;
			ben->next = firstfree;
			ben->previous = NULL;
			firstfree->previous = ben;
			firstfree = ben;
			return;
		}
	}
	else if((tail + N) == end){
		void* before = head - 2*N - ((tag*)(head - N))->size;
		payload* beforepayload = (payload*)(before + N);
		if(((tag*)(before))->taken == 1){
			size_t newsize = ((tag*)(before))->size + 2*N + ((tag*)(head))->size;
			((tag*)before)->size = newsize;
			((tag*)tail)->size = newsize;
			((tag*)tail)->taken = 1;
			return;
		}
		else{
			((tag*)head)->taken = 1;
			((tag*)tail)->taken = 1;
			ben->next = firstfree;
			ben->previous = NULL;
			firstfree->previous = ben;
			firstfree = ben;
			return;
		}
	}
	else{
		void* before = head - 2*N - ((tag*)(head - N))->size;
		void* after = tail + N;
		if(((tag*)before)->taken == 1){
			if(((tag*)after)->taken == 1 ){
				size_t newsize = ((tag*)before)->size + ((tag*)after)->size + ((tag*)head)->size + 4*N;
				((tag*)before)->size = newsize;
				((tag*)(after + N + ((tag*)after)->size))->size = newsize;
				payload* afterpayload = (payload*)(after + N);
				if(afterpayload == firstfree){
					(firstfree->next)->previous = NULL;
					firstfree = firstfree->next;
				}
				else if(afterpayload->next == NULL){
					(afterpayload->previous)->next = NULL;
				}
				else{
					(afterpayload->next)->previous = afterpayload->previous;
					(afterpayload->previous)->next = afterpayload->next;
				}
				return;
			}
			else{
				size_t newsize = ((tag*)before)->size + ((tag*)head)->size + 2*N;
				((tag*)before)->size = newsize;
				((tag*)tail)->size = newsize;
				((tag*)tail)->taken = 1;
				return;
			}
		}
		if(((tag*)after)->taken == 1){
			payload* afterpayload = (payload*)(after + N);
			if(afterpayload == firstfree){
				if(firstfree->next == NULL){
					firstfree = NULL;
				}
				else{
					(firstfree->next)->previous = NULL;
					firstfree = firstfree->next;
				}
			}
			else if(afterpayload->next == NULL){
				(afterpayload->previous)->next = NULL;
			}
			else{
				(afterpayload->next)->previous = afterpayload->previous;
				(afterpayload->previous)->next = afterpayload->next;
			}
			if(firstfree == NULL){
				firstfree = ben;
				((tag*)head)->taken = 1;
				((tag*)tail)->taken = 1;
				ben->next = NULL;
				ben->previous = NULL;
				size_t newsize = ((tag*)after)->size + 2*N + ((tag*)head)->size;
				((tag*)head)->taken = 1;
				((tag*)head)->size = newsize;
				((tag*)(after + N + ((tag*)after)->size))->size = newsize;

				return;
			}
			ben->next = firstfree;
			firstfree->previous = ben;
			ben->previous = NULL;
			firstfree = ben;
			size_t newsize = ((tag*)after)->size + 2*N + ((tag*)head)->size;
			((tag*)head)->taken = 1;
			((tag*)head)->size = newsize;
			((tag*)(after + N + ((tag*)after)->size))->size = newsize;
			return;
		}
		((tag*)head)->taken = 1;
		((tag*)tail)->taken = 1;
		ben->next = firstfree;
		ben->previous = NULL;
		firstfree->previous = ben;
		firstfree = ben;
		return;
	}
}
void* realloc(void* ptr, size_t size){
	if(!ptr)
		return malloc(size);
	if(size == 0)
		return ptr;
	void* head = ptr - N;
	size_t originalsize = ((tag*)head)->size;
	if(originalsize >= size){
		if(originalsize - size > 100000){

			void* tail1 = ptr + size;
			((tag*)head)->size = size;
			((tag*)tail1)->size = size;
			((tag*)head)->taken = 2;
			((tag*)tail1)->taken = 2;
			void* head2 = tail1 + N;
			void* tail2 = ptr + originalsize;
			((tag*)head2)->size = originalsize - size - 2*N;
			((tag*)tail2)->size = originalsize - size - 2*N;
			free(head2 + N);
		}
		return ptr;
	}
	else{
	        void* newptr = malloc(size);
		memcpy(newptr, ptr, originalsize);
	        free(ptr);
	return newptr;
	}
}	
			

		
