#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/*
	Test case
	7 4 3 14
	0 1 5 0 3 4 0 1 0 3 4 5 3 4


	5 4 3 10
	2 2 3 1 2 4 2 4 0 3


	15 5 4 30
	0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 5 6 7 5 6 7 1 2 3 4 5 6 7 8 14
*/
#define MAX_PAGE 20 
#define MAX_PAGE_FRAME 20 
#define MAX_STR_LEN 100

// MAX_PAGE: N / MAX_PAGE_FRAME: M / MAX_STR_LEN: K

int N, M, W, K;
int ref_str[MAX_STR_LEN];
int result[MAX_STR_LEN][MAX_PAGE_FRAME];
int ws_result[MAX_STR_LEN][MAX_PAGE];


void print_FA(int pfcnt, char policy[], char pf[]) {
	printf("[%s algorithm output]\n", policy);
	printf("Total Page Fault counts: %d\n", pfcnt);
	printf("+------+------------+------------+---------------------------------------------------------------\n");
	printf("| Time | Ref string | Page fault |     Memory state (%d page frames allocated)   \n", M);
	printf("+------+------------+------------+---------------------------------------------------------------\n");
	for (int i = 0; i < K; i++) {
		printf("|  %2d  |     %2d     |      %c     |"
			, i+1, ref_str[i], pf[i]);
		// print memory state
		for (int j = 0; j < M; j++) {
			if (result[i][j] == -1) {
				printf("      |");
			}
			else {
				printf("  %2d  |", result[i][j]);
			}
		}
		printf("\n");
		puts("+------+------------+------------+------------------------------------------------------------------");
	}
	printf("\n");
}

void print_VA(int pfcnt, int p_in[], int p_out[]) {
	printf("[Working set algorithm output (Windows size: %d)]\n", W);
	printf("Total Page Fault counts: %d\n", pfcnt);
	puts("+------+--------+------------+---------------------------------------------------------------------------------------------------------");
	puts("| Time |  Ref.  | Page fault |   # of  |    Memory state for each page  ");
	puts("|      | string |------------|  frames |-----------------------------------------------------------------------------------------------");
	printf("|      |        |   P_ws|Q_ws|allocated|");
	for (int j = 0; j < N; j++) {
		printf("  %2d |", j);
	}
	printf("\n");
	puts("+======+========+============+=========+================================================================================================");
	for (int i = 0; i < K; i++) {
		printf("|  %2d  |   %2d   |", i + 1, ref_str[i]);

		if (p_in[i] != -1) printf(" F  %2d |", p_in[i]);
		else printf("     - |");
		if (p_out[i] != -1) printf(" %2d |", p_out[i]);
		else printf(" -  |");

		int cnt = 0;
		for (int j = 0; j < N; j++) {
			if (ws_result[i][j] != -1) {
				cnt++;
			}
		}
		printf("   %2d    |", cnt);

		// print memory state
		for (int j = 0; j < N; j++) {
			if (ws_result[i][j] == -1) {
				printf("     |");
			}
			else {
				printf("  %2d |", ws_result[i][j]);
			}
		}
		printf("\n");
		puts("+------+--------+------------+---------------------------------------------------------------------------------------------------------");
	}
}

// LRU algorithm
void LRU_main() {
	int memst[MAX_PAGE_FRAME];
	char page_fault[MAX_STR_LEN + 1];
	int pfcnt = 0;

	for (int i = 0; i < M; i++) {
		memst[i] = -1;
	}
	for (int i = 0; i < K; i++) {
		page_fault[i] = 'X';
	}

	// For every referenced page in reference string
	for (int cur = 0; cur < K; cur++) {
		bool isFound = false;
		int pf_idx = -1; 

		// Check if it already exists in memory
		for (int p = 0; p < M; p++) {
			if (ref_str[cur] == memst[p]) {
				isFound = true;
				pf_idx = p;
				break;
			}
		}

		// If ref_str[cur] not exists in memory, Page fault
		// Check if empty page frame exists or not
		if (!isFound) {
			page_fault[cur] = 'F';
			pfcnt++;
			// Empty page frame is found
			for (int p = 0; p < M; p++) {
				if (memst[p] == -1) {
					pf_idx = p;
					isFound = 1; 
					break;
				}
			}
		}

		// If no empty page frame, page has to be replaced (Find victim)
		if (!isFound) {
			page_fault[cur] = 'F'; // page fault occurred 
			pfcnt++;

			// find victim
			int max_db = -1; // max backward distance
			// for loop for calculating backward distance of each page in memory state
			for (int p = 0; p < M; p++) {
				int tmp_db = INT_MAX;
				for (int rt = cur - 1; rt >= 0; rt--) {
					if (ref_str[rt] == memst[p]) {
						tmp_db = cur - rt;
						break;
					}
				}
				
				if (max_db < tmp_db) {
					max_db = tmp_db;
					pf_idx = p;
				}
			}
		}

		// Place ref_cur[str] in page frame index
		if (page_fault[cur] == 'F') {
			memst[pf_idx] = ref_str[cur];
		}
		// Save results
		for (int i = 0; i < M; i++) {
			result[cur][i] = memst[i];
		}
	}
	print_FA(pfcnt, "LRU", page_fault);
}

// LFU algorithm
void LFU_main() {
	int memst[MAX_PAGE_FRAME];
	int refcnt[MAX_PAGE];
	char page_fault[MAX_STR_LEN + 1];
	int pfcnt = 0;

	for (int i = 0; i < M; i++) {
		memst[i] = -1;
	}
	for (int i = 0; i < N; i++) {
		refcnt[i] = 0;
	}
	for (int i = 0; i < K; i++) {
		page_fault[i] = 'X';
	}

	// For every referenced page in reference string
	for (int cur = 0; cur < K; cur++) {
		bool isFound = false;
		int pf_idx = -1;

		// Check if it already exists in memory
		for (int p = 0; p < M; p++) {
			if (ref_str[cur] == memst[p]) {
				isFound = true;
				pf_idx = p;
				break;
			}
		}

		// If ref_str[cur] not exists in memory, Page fault
		// Check if empty page frame exists or not
		if (!isFound) {
			page_fault[cur] = 'F';
			pfcnt++;
			// Empty page frame is found
			for (int p = 0; p < M; p++) {
				if (memst[p] == -1) {
					pf_idx = p;
					isFound = 1;
					break;
				}
			}
		}
		// If no empty page frame, page has to be replaced (Find victim)
		if (!isFound) {
			page_fault[cur] = 'F'; // page fault occurred 
			pfcnt++;
			int lfu_p = 0; // least frequently used page index in memory
			for (int p = 0; p < M; p++) {
				if (refcnt[memst[p]] < refcnt[memst[lfu_p]]) {
					lfu_p = p;
				}
				// Tie breaking rule: LRU
				else if (refcnt[memst[p]] == refcnt[memst[lfu_p]]) {
					int p_db, lfu_p_db;
					for (int rt = cur - 1; rt >= 0; rt--) {
						if (ref_str[rt] == memst[p]) {
							p_db = cur - rt;
							break;
						}
					}
					for (int rt = cur - 1; rt >= 0; rt--) {
						if (ref_str[rt] == memst[lfu_p]) {
							lfu_p_db = cur - rt;
							break;
						}
					}
					lfu_p = p_db > lfu_p_db ? p : lfu_p;
				}
			}
			pf_idx = lfu_p;

		}

		// Place ref_cur[str] in page frame index
		if (page_fault[cur] == 'F') {
			memst[pf_idx] = ref_str[cur];
		}
		refcnt[ref_str[cur]]++; // ref cnt increases 

		// Save results
		for (int i = 0; i < M; i++) {
			result[cur][i] = memst[i];
		}
	}
	print_FA(pfcnt, "LFU", page_fault);
}

// FIFO Algorithm
void FIFO_main() {
	int memst[MAX_PAGE_FRAME];
	char page_fault[MAX_STR_LEN + 1];
	int fifoQueue[MAX_STR_LEN]; // queue using array

	int rear = -1;
	int front = 0;
	int pfcnt = 0;

	/*
		FIFO queue
		1. For page replacement: displace the page at front (front++)
		2. For incoming page number: insert it to the rear (rear++)
	*/

	for (int i = 0; i < M; i++) {
		memst[i] = -1;
	}
	for (int i = 0; i < K; i++) {
		page_fault[i] = 'X';
		fifoQueue[i] = -1;
	}

	// For every referenced page in reference string
	for (int cur = 0; cur < K; cur++) {
		bool isFound = false;
		int pf_idx = -1; 

		// Check if it already exists in memory
		for (int p = 0; p < M; p++) {
			if (ref_str[cur] == memst[p]) {
				isFound = true;
				pf_idx = p;
				break;
			}
		}

		// If ref_str[cur] not exists in memory, Page fault
		// Check if empty page frame exists or not
		if (!isFound) {
			page_fault[cur] = 'F';
			pfcnt++;
			
			// Insert the page to the rear of the queue
			fifoQueue[++rear] = ref_str[cur]; 

			// Empty page frame is found
			for (int p = 0; p < M; p++) {
				if (memst[p] == -1) {
					pf_idx = p;
					isFound = 1; 
					break;
				}
			}		
		}

		// If no empty page frame, page has to be replaced (Find victim)
		if (!isFound) {
			page_fault[cur] = 'F'; 	
			pfcnt++;

			// Find the page frame index of the fifoQueue[front]
			for (int p = 0; p < M; p++) {
				if (fifoQueue[front] == memst[p]) {
					pf_idx = p;
					break;
				}
			}
			front++;
		}

		// Place ref_cur[str] in page frame index		
		if (page_fault[cur] == 'F') {
			memst[pf_idx] = ref_str[cur];
		}
		
		// Save results
		for (int i = 0; i < M; i++) {
			result[cur][i] = memst[i];
		}
	}
	print_FA(pfcnt, "FIFO", page_fault);
}

// MIN Algorithm
void MIN_main() {
	// State the memory state array, page fault array and page fault count
	int memst[MAX_PAGE_FRAME];
	char page_fault[MAX_STR_LEN+1];
	int pfcnt = 0;

	// Initialize memory state as -1 (As page number starts from 0)
	memset(memst, -1, sizeof(memst));
	
	// Initialize page fault array as 'X'
	for (int i = 0; i < K; i++) {
		page_fault[i] = 'X';
	}

	// For loop for checking every single reference string page number one by one
	for (int cur = 0; cur < K; cur++) {	
		bool isFound = false; // True if the index of memory state to load the page is chosen
		int pf_idx = -1; // Page frame index that ref_str[cur] will be loaded
		
		// Check if ref_str[cur] already exists in memory
		for (int p = 0; p < M; p++) {
			if (ref_str[cur] == memst[p]) {
				isFound = true;
				pf_idx = p;
				break;
			}
		}
		// If ref_str[cur] not exists in memory, Page fault
		// Check if empty page frame exists or not
		if (!isFound) {
			page_fault[cur] = 'F'; // page fault
			pfcnt++;
			
			// Empty page frame is found
			for (int p = 0; p < M; p++) {
				if (memst[p] == -1) {
					pf_idx = p;
					isFound = 1;
					break;
				}
			}
		}

		// If no empty page frame, page has to be replaced (Find victim)
		if (!isFound) {
			page_fault[cur] = 'F'; // page fault 
			pfcnt++;

			// Find victim by comparing forward distance
			int max_df = -1;
			// For loop for each page in memory state
			for (int p = 0; p < M; p++) {	
				// Covers the case when the page is not referenced anymore
				int tmp_df = INT_MAX;  
				for (int rt = cur + 1; rt < K; rt++) {
					if (ref_str[rt] == memst[p]) {
						tmp_df = rt - cur;
						break;
					}
				}
				// Update max_df if tmp_df is larger than max_df
				if (max_df < tmp_df) {
					max_df = tmp_df;
					pf_idx = p;
				}
				// Tie breaking rule: page with smallest number
				if (max_df == tmp_df) {			
					pf_idx = memst[p] < memst[pf_idx] ? p : pf_idx;
				}
			}
		}

		// Place ref_cur[str] in page frame index
		if (page_fault[cur] == 'F') {
			memst[pf_idx] = ref_str[cur]; 
		}

		// Save the result of time==cur
		for (int i = 0; i < M; i++) {
			result[cur][i] = memst[i];
		}
	}
	print_FA(pfcnt, "MIN", page_fault);
}

// Working set
void WS_main() {
	int memst[MAX_PAGE];
	int page_out[MAX_STR_LEN];
	int page_in[MAX_STR_LEN];
	int pfcnt = 0;

	for (int i = 0; i < N; i++) {
		memst[i] = -1;
	}
	for (int i = 0; i < K; i++) {
		page_out[i] = -1;
		page_in[i] = -1;
	}

	// For loop for checking every single reference string page number one by one
	for (int cur = 0; cur < K; cur++) {
		/*
			tmp_p_ws: ref_str[cur-W-1] -> Page that may be loaded or not
			tmp_q_ws: ref_str[cur] -> Page that may be displaced or not

			1. Check if ref_str[cur] exists in previous working set(ref_str[n-1]~ref_str[n-W-1])
				If exists, pass
				If not, page fault and ref_str[cur] loaded in memory
			2. Check if ref_str[cur-W-1] exists in now working set(ref_str[n]~ref_str[n-W])
				If exists, pass
				If not, displaced
		*/

		int tmp_p_ws, tmp_q_ws = ref_str[cur];
		int isIn_p = 0, isIn_q = 0; // 0 if not exists, 1 if exists

		// 1. Check if ref_str[cur] exists in previous working set(ref_str[n-1]~ref_str[n-W-1])
		for (int p = cur; p >= 0 && p >= cur - W - 1; p--) {
			if (p != cur && tmp_q_ws == ref_str[p]) {
				isIn_q = 1;
				break;
			}
		}
		// Page fault
		if (!isIn_q) {
			memst[tmp_q_ws] = tmp_q_ws;
			page_in[cur] = tmp_q_ws;
			pfcnt++;
		}

		// 2. Check if ref_str[cur-W-1] exists in now working set(ref_str[n]~ref_str[n-W])
		if (cur - W - 1 >= 0) {
			tmp_p_ws = ref_str[cur - W - 1];
			for (int p = cur; p >= cur - W; p--) {
				if (p != cur - W && tmp_p_ws == ref_str[p]) {
					isIn_p = 1;
					break;
				}
			}
			// Displaced
			if (!isIn_p) {
				memst[tmp_p_ws] = -1;
				page_out[cur] = tmp_p_ws;
			}
		}
		
		/*for (int i = 0; i < N; i++) {
			printf("%d: %d // ", i, memst[i]);
		}
		printf("%d / %d ==> pfcnt: %d\n", page_in[cur], page_out[cur], pfcnt);*/

		// Save results
		for (int i = 0; i < N; i++) {
			ws_result[cur][i] = memst[i];	
		}
	}
	print_VA(pfcnt, page_in, page_out);
}

int main() {
	
	FILE* fptr = fopen("inputvm.txt", "r");
	fscanf(fptr, "%d %d %d %d", &N, &M, &W, &K);
	for (int i = 0; i < K; i++) {
		fscanf(fptr, "%d", &ref_str[i]);
	}
	fclose(fptr);
	
	MIN_main();
	FIFO_main();
	LRU_main();
	LFU_main();
	WS_main();

	return 0;
}
