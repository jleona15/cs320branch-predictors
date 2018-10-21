#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TAKEN 1
#define NOT_TAKEN 0

#define STRONGLY_TAKEN 3
#define WEAKLY_TAKEN 2
#define WEAKLY_NOT_TAKEN 1
#define STRONGLY_NOT_TAKEN 0

#define PREFER_GSHARE 0
#define WEAKLY_PREFER_GSHARE 1
#define WEAKLY_PREFER_BIMODAL 2
#define PREFER_BIMODAL 3

#define MULTI_G_WIDTH 2
#define MULTI_G_LENGTH 1024
#define G_MASK_START 4
#define G_MUL 3

#define PERCEPTRON_CYCLES 1
#define P_TABLE_SIZE 4096
#define MAX_WEIGHT 0

#define TAGE_BIMODAL_SIZE 2048
#define TAGE_ENTRIES 1024
#define TAGE_CTR_BITS 12
#define TAGE_TAG_BITS 12

FILE *input = NULL;
FILE *output = NULL;
char *buff;
size_t buff_size = 256;

int prog_argc;
char o_path[256];
char i_path[256];

///
FILE *special_output = NULL;

struct s_t_entry {
	int ctr;
	int useful;
	int tag;
};

typedef struct s_t_entry t_entry;

void open_input() {
	if(input != NULL) {
		fclose(input);
		input = NULL;
	}
	
	if(prog_argc == 1 || prog_argc == 2) {
		fprintf(stderr, "Error, file name not specified on command line\n");
		exit(0);
	} else if (prog_argc != 3) {
		fprintf(stderr, "Error, too many arguments specified\n");

		exit(0);
	}

	
	input = fopen(i_path, "r");

	if(input == NULL) {
		fprintf(stderr, "Error, file name specified is not valid\n");
		exit(0);
	}
}

//Determines the address of the trace and the taken flag from a single line of
//input, returning them through the pointers passed in
void input_line(unsigned long *addr, int *taken, int *eof) {
	if(getline(&buff, &buff_size, input) == -1) {
		*eof = 1;
	} else {
		*eof = 0;
	
		int scale = 1;
		*addr = 0;
	
		//Converts the string for the address into an integer
		for(int i = 7; i >= 0; --i) {
			if(buff[i+2] >= 48 && buff[i+2] <= 57) {
				*addr += scale * (buff[i+2] - 48);
			} else if (buff[i+2] >= 97 && buff[i+2] <= 102) {
				*addr += scale * (buff[i+2] - 87);
			}
	
			scale *= 16;
		}

		if(strcmp(buff+11, "NT\n") == 0) {
			*taken = NOT_TAKEN;
		} else {
			*taken = TAKEN;
		}
	}
}

//An always taken prediction algorithm
void always_taken() {
	open_input();

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(taken == TAKEN) {
			++correct_count;
		}

		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;\n", correct_count, total_count);
}

//An always not taken prediction algorithm
void always_not_taken() {
	open_input();

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(taken == NOT_TAKEN) {
			++correct_count;
		}

		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;\n", correct_count, total_count);
}

void bimodal_one_bit(int table_size) {
	int table[table_size];

	for(int i = 0; i < table_size; ++i) {
		table[i] = TAKEN;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(taken == table[addr % table_size]) {
			++correct_count;
		}

		if(taken == TAKEN && table[addr % table_size] < TAKEN) {
			++table[addr % table_size];
		} else if(taken == NOT_TAKEN && table[addr % table_size] > NOT_TAKEN) {
			--table[addr % table_size];
		}

		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;", correct_count, total_count);
}

void bimodal_two_bit(int table_size) {
	int table[table_size];

	for(int i = 0; i < table_size; ++i) {
		table[i] = STRONGLY_TAKEN;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(taken == (table[addr % table_size] >> 1)) {
			++correct_count;
		}

		if(taken == TAKEN && table[addr % table_size] < STRONGLY_TAKEN) {
			++table[addr % table_size];
		} else if(taken == NOT_TAKEN && table[addr % table_size] > STRONGLY_NOT_TAKEN) {
			--table[addr % table_size];
		}

		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;", correct_count, total_count);
}

void gshare(int history_bits) {
	int table[2048];

	int mask = (1 << history_bits) - 1;

	int ghr = 0;

	for(int i = 0; i < 2048; ++i) {
		table[i] = STRONGLY_TAKEN;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(taken == (table[ghr ^ (addr%2048)] >> 1)) {
			++correct_count;
		}

		if(taken == TAKEN && table[ghr ^ (addr%2048)] < STRONGLY_TAKEN) {
			++table[ghr ^ (addr%2048)];
		} else if(taken == NOT_TAKEN && table[ghr ^ (addr%2048)] > STRONGLY_NOT_TAKEN) {
			--table[ghr ^ (addr%2048)];
		}

		ghr = mask & ((ghr << 1) | taken);
		
		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;", correct_count, total_count);
}

void tournament() {
	int g_table[2048];
	int b_table[2048];
	int s_table[2048];

	int mask = (1 << 11) - 1;

	int ghr = 0;

	for(int i = 0; i < 2048; ++i) {
		g_table[i] = STRONGLY_TAKEN;
	}

	for(int i = 0; i < 2048; ++i) {
		b_table[i] = STRONGLY_TAKEN;
	}

	for(int i = 0; i < 2048; ++i) {
		s_table[i] = PREFER_GSHARE;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		if(s_table[addr%2048] <= WEAKLY_PREFER_GSHARE) {
			if(taken == (g_table[ghr ^ (addr%2048)] >> 1)) {
				++correct_count;
				if(taken != (b_table[addr%2048] >> 1)) {
					if(s_table[addr%2048] == WEAKLY_PREFER_GSHARE) {
						s_table[addr%2048] = PREFER_GSHARE;
					}	
				}
			} else {
				if(taken == (b_table[addr%2048] >> 1))  {
					if(s_table[addr%2048] == WEAKLY_PREFER_GSHARE) {
						s_table[addr%2048] = WEAKLY_PREFER_BIMODAL;
					} else {
						s_table[addr%2048] = WEAKLY_PREFER_GSHARE;
					}
				}
			}
		} else {
			if(taken == (b_table[addr%2048] >> 1)) { 
				++correct_count;
				if(taken != (g_table[(ghr ^ addr)%2048] >> 1)) {
					if(s_table[addr%2048] == WEAKLY_PREFER_BIMODAL) {
						s_table[addr%2048] = PREFER_BIMODAL;
					}
				}
			} else {
				if(taken == (g_table[ghr ^ (addr%2048)] >> 1)) {
					if(s_table[addr%2048] == WEAKLY_PREFER_BIMODAL) {
						s_table[addr%2048] = WEAKLY_PREFER_GSHARE;
					} else {
						s_table[addr%2048] = WEAKLY_PREFER_BIMODAL;
					}
				}
			}
			
		}
		//if(s_table[addr%2048] == PREFER_GSHARE || s_table[addr%2048] == WEAKLY_PREFER_GSHARE) {
			if(taken == TAKEN && g_table[ghr ^ (addr%2048)] < STRONGLY_TAKEN) {
				++g_table[ghr ^ (addr%2048)];
			} else if(taken == NOT_TAKEN && g_table[ghr ^ (addr%2048)] > STRONGLY_NOT_TAKEN) {
				--g_table[ghr ^ (addr%2048)];
			}
		//} else {
			if(taken == TAKEN && b_table[addr % 2048] < STRONGLY_TAKEN) {
				++b_table[addr % 2048];
			} else if(taken == NOT_TAKEN && b_table[addr % 2048] > STRONGLY_NOT_TAKEN) {
				--b_table[addr % 2048];
			}
		//}

		ghr = mask & ((ghr << 1) | taken);
		
		input_line(&addr, &taken, &eof);
	}	

	fprintf(output, "%i,%i;\n", correct_count, total_count);
}

//EC
void tournament_plus(void) {
	int g_tables[MULTI_G_LENGTH][MULTI_G_WIDTH];
	int b_table[MULTI_G_LENGTH];
	int s_table[MULTI_G_LENGTH];

	//int mask = (1 << 11) - 1;

	int ghr = 0;

	int selection;

	int g_sum;
	int g_select;

	int b_select;

	for(int i = 0; i < MULTI_G_WIDTH; ++i) {
		for(int i2 = 0; i2 < MULTI_G_LENGTH; ++i2) {
			g_tables[i2][i] = STRONGLY_TAKEN;
		}	
	}

	for(int i = 0; i < MULTI_G_LENGTH; ++i) {
		b_table[i] = STRONGLY_TAKEN;
	}

	for(int i = 0; i < MULTI_G_LENGTH; ++i) {
		s_table[i] = PREFER_GSHARE;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		selection = s_table[addr%MULTI_G_LENGTH];

		g_sum = 0;

		for(int i = 0; i < MULTI_G_WIDTH; ++i) {
			g_sum += g_tables[((ghr & ((1 << (G_MASK_START+(i*G_MUL)))-1)) ^ (addr%MULTI_G_LENGTH))%MULTI_G_LENGTH][i];
		}

		if(((double)g_sum / MULTI_G_WIDTH) >= 1.5) {
			g_select = TAKEN;
		} else {
			g_select = NOT_TAKEN;
		}

		b_select = b_table[addr%MULTI_G_LENGTH] >> 1;

		if(selection <= WEAKLY_PREFER_GSHARE) {
			if(taken == g_select) {
				++correct_count;
				if(taken != b_select) {
					if(s_table[addr%MULTI_G_LENGTH] == WEAKLY_PREFER_GSHARE) {
						s_table[addr%MULTI_G_LENGTH] = PREFER_GSHARE;
					}	
				}
			} else {
				if(taken == b_select)  {
					if(s_table[addr%MULTI_G_LENGTH] == WEAKLY_PREFER_GSHARE) {
						s_table[addr%MULTI_G_LENGTH] = WEAKLY_PREFER_BIMODAL;
					} else {
						s_table[addr%MULTI_G_LENGTH] = WEAKLY_PREFER_GSHARE;
					}
				}
			}
		} else {
			if(taken == b_select) { 
				++correct_count;
				if(taken != g_select) {
					if(s_table[addr%MULTI_G_LENGTH] == WEAKLY_PREFER_BIMODAL) {
						s_table[addr%MULTI_G_LENGTH] = PREFER_BIMODAL;
					}
				}
			} else {
				if(taken == g_select) {
					if(s_table[addr%MULTI_G_LENGTH] == WEAKLY_PREFER_BIMODAL) {
						s_table[addr%MULTI_G_LENGTH] = WEAKLY_PREFER_GSHARE;
					} else {
						s_table[addr%MULTI_G_LENGTH] = WEAKLY_PREFER_BIMODAL;
					}
				}
			}
			
		}
		if(taken == TAKEN) {
			for(int i = 0; i < MULTI_G_WIDTH; ++i) {
				if(g_tables[((ghr & ((1 << (G_MASK_START+(i*G_MUL)))-1)) ^ (addr%MULTI_G_LENGTH))%MULTI_G_LENGTH][i] < STRONGLY_TAKEN) {
				++g_tables[((ghr & ((1 << (G_MASK_START+(i*G_MUL)))-1)) ^ (addr%MULTI_G_LENGTH))%MULTI_G_LENGTH][i];
				}
			}
		} else { 
			for(int i = 0; i < MULTI_G_WIDTH; ++i) {
				if(g_tables[((ghr & ((1 << (G_MASK_START+(i*G_MUL)))-1)) ^ (addr%MULTI_G_LENGTH))%MULTI_G_LENGTH][i] > STRONGLY_NOT_TAKEN) {
				--g_tables[((ghr & ((1 << (G_MASK_START+(i*G_MUL)))-1)) ^ (addr%MULTI_G_LENGTH))%MULTI_G_LENGTH][i];
				}
			}
		}

		if(taken == TAKEN) {
			++b_table[addr % MULTI_G_WIDTH];
		} else { 
			--b_table[addr % MULTI_G_WIDTH];
		}

		ghr = (ghr << 1) | taken;
		
		input_line(&addr, &taken, &eof);
	}	

	fprintf(special_output, "%i,%i;\n", correct_count, total_count);
}

//EC 
void perceptron(void) {
	int perceptrons[P_TABLE_SIZE][33];

	int inputs[32];

	int ghr = 0;


	for(int i = 0; i < 33; ++i) {
		for(int i2 = 0; i2 < P_TABLE_SIZE; ++i2) {
			perceptrons[i2][i] = 0;
		}
	}

	int p_select;
	int p_sum;

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;

	int temp_ghr;	

	int p;

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		p_sum = 0;

		p = addr%P_TABLE_SIZE;
		
		temp_ghr = ghr;

		for(int i = 0; i < 32; ++i) {
			if((temp_ghr % 2) == 1) {
				inputs[i] = 1;
			} else {
				inputs[i] = -1;
			}
			temp_ghr = temp_ghr / 2;
		}

		/*for(int i = 32; i < 64; ++i) {
			if((temp_ghr % 2) == 1) {
				inputs[i] = 1;
			} else {
				inputs[i] = -1;
			}
			temp_ghr = temp_ghr / 2;
		}*/

		for(int i = 0; i < 32; ++i) {
			p_sum += inputs[i] * perceptrons[p][i];
		}

		p_sum += perceptrons[p][32];

		if(p_sum >= 0) {
			p_select = TAKEN;
		} else {
			p_select = NOT_TAKEN;
		}

		if(taken == p_select) {
			++correct_count;
		} else { 
			

			/*int b_mod;
			int mod_i;
			int mod_op;
			int p_sum_prev = p_sum;
			int b_prev = bias;*/

			int current_cycles = 0;
			
			while(p_select != taken && (PERCEPTRON_CYCLES == 0 || current_cycles < PERCEPTRON_CYCLES)) {

				++current_cycles;

				for(int i = 0; i < 33; ++i) {
					if(taken == TAKEN) {
						perceptrons[p][i] += inputs[i];
					} else {
						perceptrons[p][i] += -1 * inputs[i];
					}

					if((perceptrons[p][i] > MAX_WEIGHT) && (MAX_WEIGHT != 0)) {
						perceptrons[p][i] = MAX_WEIGHT;
					} else if((perceptrons[p][i] < MAX_WEIGHT) && (MAX_WEIGHT != 0)) {
						perceptrons[p][i] = MAX_WEIGHT;
					}
				}
				//++current_cycles;

				/*mod_op = rand() % 2;

				if((rand() % 10) < 8) {
					b_mod = 0;
				} else {
					b_mod = 1;
				}

				if(b_mod == 0) {
					mod_i = rand() % 64;

					if(mod_op == 1) {
						weights[mod_i] += 1;
					} else {
						weights[mod_i] -= 1;
					}
				} else {
					if(mod_op == 1) {
						bias += 1;
					} else {
						bias -= 1;
					}
				}*/

				p_sum = 0;

				for(int i = 0; i < 32; ++i) {
					p_sum += inputs[i] * perceptrons[p][i];
				}

				p_sum += perceptrons[p][32];
				/*
				if(((taken == TAKEN) && (p_sum-bias > p_sum_prev-b_prev)) || ((taken == NOT_TAKEN) && (p_sum-bias < p_sum_prev-b_prev))) {
					b_prev = bias;
					p_sum_prev = p_sum;
					++current_cycles;	
				} else {
					if(b_mod == 1) {
						if(mod_op == 1) {
							bias -= 1;
						} else {
							bias += 1;
						}
					} else {
						if(mod_op == 1) {
							weights[mod_i] -= 1;
						} else {
							weights[mod_i] += 1;
						}
					}
				}*/

				if(p_sum >= 0) {
					p_select = TAKEN;
				} else {
					p_select = NOT_TAKEN;
				}
			}
		}

		

		ghr = ((ghr << 1) | taken) ;
		
		input_line(&addr, &taken, &eof);
	}	

	fprintf(special_output, "%i,%i;", correct_count, total_count);

}

//EC
void tage(void) {
	int num_tables = 8;

	int b_table[TAGE_BIMODAL_SIZE];
	t_entry t_table[num_tables][TAGE_ENTRIES];

	for(int i = 0; i < TAGE_BIMODAL_SIZE; ++i) {
		b_table[i] = STRONGLY_TAKEN;
	}

	for(int i1 = 0; i1 < num_tables; ++i1) {
		for(int i = 0; i < TAGE_ENTRIES; ++i) {
			t_table[i1][i].ctr = 0;
			t_table[i1][i].useful = 0;
			t_table[i1][i].tag = 0;
		}
	}

	int ghr = 0;

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;

	int tag;
	int matching_index1;
	int matching_index2;
	int rolling_index[num_tables];

	for(int i = 0; i < num_tables; ++i) {
		rolling_index[i] = 0;
	}

	int prediction;

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		tag = (addr & ((1<<TAGE_TAG_BITS)-1)) ^ ghr;

		matching_index1 = -1;
		matching_index2 = -1;

		for(int i1 = 0; i1 < num_tables; ++i1) {
			for(int i = 0; i < TAGE_ENTRIES; ++i) {
				if(t_table[i1][i].tag == tag) {
					matching_index1 = i1;
					matching_index2 = i;
					break;
				}
			}

			if(matching_index1 != -1) break;
		}

		if(matching_index1 == -1) {
			prediction = b_table[addr % TAGE_BIMODAL_SIZE] >> 1;	
		} else {
			prediction = t_table[matching_index1][matching_index2].ctr >> 2;
		}

		if(taken == prediction) {
			++correct_count;
		}

		if(matching_index1 == -1) {
			for(int i = 0; i < num_tables; ++i) {
				int bound = rolling_index[i]-1;

				if(bound == -1) { bound = TAGE_ENTRIES; }

				for(; rolling_index[i] != bound; ++rolling_index[i]) {
					if(t_table[i][rolling_index[i]].useful == 0) {
						t_table[i][rolling_index[i]].ctr = 3;
						t_table[i][rolling_index[i]].useful = 1;
						t_table[i][rolling_index[i]].tag = tag;
						break;
					} else {
						t_table[i][rolling_index[i]].useful = 0;
					}

					if(rolling_index[i] >= TAGE_ENTRIES) {
						rolling_index[i] = 0;
					}
				}
			}
		} else {
			if((taken == TAKEN) && (t_table[matching_index1][matching_index2].ctr < ((1<<3)-1))) {
				++t_table[matching_index1][matching_index2].ctr;
			} else if ((taken == NOT_TAKEN) && (t_table[matching_index1][matching_index2].ctr > 0)){ 
				--t_table[matching_index1][matching_index2].ctr;
			}
		}

		if(taken == TAKEN && b_table[addr % TAGE_ENTRIES] < STRONGLY_TAKEN) {
			++b_table[addr % TAGE_ENTRIES];
		} else if(taken == NOT_TAKEN && b_table[addr % TAGE_ENTRIES] > STRONGLY_NOT_TAKEN) {
			--b_table[addr % TAGE_ENTRIES];
		}



		ghr = (ghr << 1) | taken;

		input_line(&addr, &taken, &eof);
	}
	
	fprintf(special_output, "%i,%i;", correct_count, total_count);
}

//EC
void local() {
	int history_size = 511;
	int counts_size = 253;

	int history[history_size];
	int counts[counts_size];

	for(int i = 0; i < history_size; ++i) {
		history[i] = NOT_TAKEN;
	}

	for(int i = 0; i < counts_size; ++i) {
		counts[i] = STRONGLY_TAKEN;
	}

	int correct_count = 0;
	int total_count = 0;

	unsigned long addr;
	int taken;
	int eof;	

	int index;

	open_input();

	input_line(&addr, &taken, &eof);

	while(eof != 1) {
		++total_count;

		//index = ((((addr << 16)) >> 16) ^ (addr >> 16)) % history_size;
		index = addr % history_size;

		if(taken == (counts[history[index]%counts_size] >> 1)) {
			++correct_count;
		}

		if(taken == TAKEN) {
			if(counts[history[index]%counts_size] < STRONGLY_TAKEN) {
				++counts[history[index]%counts_size];
			}
			history[index] = (history[index] << 1) | 1;
			
		} else {
			if(counts[history[index]%counts_size] > STRONGLY_NOT_TAKEN) {
				--counts[history[index]%counts_size];
			}
			history[index] = (history[index] << 1);
			
		}
		
		input_line(&addr, &taken, &eof);
	}	

	fprintf(special_output, "%i,%i;", correct_count, total_count);
}


int main(int argc, char *argv[]) {

	prog_argc = argc;

	//EC
	//special_output = fopen("special_output.txt", "w");

	if(prog_argc == 1 || prog_argc == 2) {
		fprintf(stderr, "Error, file names not specified on command line\n");
		exit(0);
	} else if (prog_argc != 3) {
		fprintf(stderr, "Error, too many arguments specified\n");

		exit(0);
	}

	strcpy(o_path, argv[2]);
	
	strcpy(i_path, argv[1]);
	
	open_input();	

	output = fopen(o_path, "w");

	if(output == NULL) {
		fprintf(stderr, "Error, file name specified is not valid\n");
		exit(0);
	}

	buff = calloc(1, 256);
	
	//Start of prediction algorithms
	
	//Always taken
	always_taken();

	//Always not taken
	always_not_taken();

	//Bimodal one bit
	bimodal_one_bit(16);

	fprintf(output, " ");

	bimodal_one_bit(32);

	for(int i = 128; i <= 2048; i*=2) {
		fprintf(output, " ");
		bimodal_one_bit(i);	
	}

	fprintf(output, "\n");

	//Bimodal two bit
	bimodal_two_bit(16);

	fprintf(output, " ");

	bimodal_two_bit(32);

	for(int i = 128; i <= 2048; i*=2) {
		fprintf(output, " ");
		bimodal_two_bit(i);	
	}

	fprintf(output, "\n");

	//Gshare
	
	gshare(3);

	for(int i = 4; i <= 11; ++i) {
		fprintf(output, " ");
		gshare(i);
	}

	fprintf(output, "\n");

	//Tournament
	tournament();

	//End of program cleanup

	free(buff);

	fclose(output);

	//EC
	//fclose(special_output);
	
	if(input != NULL) {
		fclose(input);
	}
}
