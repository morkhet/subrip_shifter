#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef struct {
	int8_t hours;
	int8_t minutes;
	int8_t seconds;
	int16_t milliseconds;
} subrip_time;

typedef struct{
	uint16_t index;
	subrip_time start_time;
	subrip_time end_time;
	char *text;
	uint16_t length;
} subrip_part;


typedef enum{
	LINETYPE_INDEX,
	LINETYPE_TIME,
	LINETYPE_TEXT,
	LINETYPE_END
} line_type;

int usage(){
	printf("Usage: subrip_shifter -i <input_file> -o <output_file> (-a <milliseconds> | -s <milliseconds>)\n");
	printf("\n");
	printf("-i <input_file>			input file which contains the subrip data\n");
	printf("-o <output_file>		output file which will store the changed data\n");
	printf("-a <milliseconds>		linear add time to each time-value\n");
	printf("-s <milliseconds>		linear substract time to each time-value\n");
}

int subrip_line_to_time(char **line, uint8_t start_index, subrip_time *time){
	if(!line || !*line || !time)
		return 0;
	//lineFormat = "00:00:00,000 --> 00:00:00,000" hours:minutes:seconds,milliseconds
	
	char num2[3]; //buffer for 2-digit int
	char num3[4]; //buffer for 3-digit int
	//hours
	memcpy(num2, *line+start_index+0,2);
	num2[3] = '\0';
	time->hours = atoi(num2);
	//minutes
	memcpy(num2, *line+start_index+3,2);
	num2[3] = '\0';
	time->minutes = atoi(num2);
	//seconds
	memcpy(num2, *line+start_index+6,2);
	num2[3] = '\0';
	time->seconds = atoi(num2);
	//milliseconds
	memcpy(num3, *line+start_index+9,3);
	num3[3] = '\0';
	time->milliseconds = atoi(num3);
}
int is_valid_subrip_index_line(char **line, int16_t line_size){
	if(!line || !*line)
		return 0;
	int8_t has_number = 0;
	for(int8_t i = line_size-1; i >= 0; --i){
		if(i == line_size-1){
			if(*(*line+i) != '\n')
				return 0; //last characters must be \n
			continue;
		}
		if(*(*line+i) < 48 || *(*line+i) > 57){
			if(i == line_size-2 && *(*line+i) == '\r')
				continue; //second last character could be \r
			return 0;
		} else {
			has_number = 1;
		}
	}
	return has_number;
}
int is_valid_subrip_time_line(char **line, int16_t line_size){
	//lineFormat = "00:00:00,000 --> 00:00:00,000" hours:minutes:seconds,milliseconds
	int8_t valid_end = 0;
	if(!line || !*line)
		return 0;
	if(line_size != 31 && line_size != 30){
		return 0;
	}
	//we are doing this hard-coded.
	if(line_size == 30){
		valid_end = *(*line+29) == '\n';
	}
	if(line_size == 31){
		valid_end = (*(*line+29) == '\r' && *(*line+30) == '\n');
	}
	return (*(*line) >= 48 && *(*line) <= 57) && // 0-9
		(*(*line+1) >= 48 && *(*line+1) <= 57) && // 0-9
		(*(*line+2) == 58) && //":"
		(*(*line+3) >= 48 && *(*line+3) <= 57) && //0-9
		(*(*line+4) >= 48 && *(*line+4) <= 57) && //0-9
		(*(*line+5) == 58) && //":"
		(*(*line+6) >= 48 && *(*line+6) <= 57) && //0-9
		(*(*line+7) >= 48 && *(*line+7) <= 57) && //0-9
		(*(*line+8) == 44) && //","
		(*(*line+9) >= 48 && *(*line+9) <= 57) && //0-9
		(*(*line+10) >= 48 && *(*line+10) <= 57) && //0-9
		(*(*line+11) >= 48 && *(*line+11) <= 57) && // 0-9
		(*(*line+12) == 32) && //" "
		(*(*line+13) == 45) && //"-"
		(*(*line+14) == 45) && //"-"
		(*(*line+15) == 62) && //">"
		(*(*line+16) == 32) && //" "
		(*(*line+17) >= 48 && *(*line+1) <= 57) && // 0-9
		(*(*line+18) >= 48 && *(*line+1) <= 57) && // 0-9
		(*(*line+19) == 58) && //":"
		(*(*line+20) >= 48 && *(*line+3) <= 57) && //0-9
		(*(*line+21) >= 48 && *(*line+4) <= 57) && //0-9
		(*(*line+22) == 58) && //":"
		(*(*line+23) >= 48 && *(*line+6) <= 57) && //0-9
		(*(*line+24) >= 48 && *(*line+7) <= 57) && //0-9
		(*(*line+25) == 44) && //","
		(*(*line+26) >= 48 && *(*line+9) <= 57) && //0-9
		(*(*line+27) >= 48 && *(*line+10) <= 57) && //0-9
		(*(*line+28) >= 48 && *(*line+11) <= 57) && // 0-9
		valid_end;
}
int is_valid_subrip_text_line(char **line, int16_t line_size){
	if(!line || !*line)
		return 0;
	for(int8_t i = line_size-1; i >= 0; --i){
		if(i == line_size-1){
			if(*(*line+i) != '\n')
				return 0; //last characters must be \n
			continue;
		}
		//we really dont care about encoding here
		/*if(*(*line+i) < 32 || *(*line+i) > 126){ //every printable character ascii
			if(i == line_size-2 && *(*line+i) == '\r')
				continue; //second last character could be \r
			return 0;
		}*/
	}
}
int read_subrip_from_line(char **line, int16_t line_size, subrip_part *result, line_type *type){
	if(!line || !*line || !result || !type){
		return 0;
	}
	if(*type == LINETYPE_INDEX){
		if(!is_valid_subrip_index_line(line, line_size)){
			return 0;
		}
		result->index = atoi(*line);
		return 1;
	}
	if(*type == LINETYPE_TIME){
		if(!is_valid_subrip_time_line(line,line_size)){
			return 0;
		}
		subrip_line_to_time(line, 0, &(result->start_time));
		subrip_line_to_time(line, 17, &(result->end_time));
		return 1;
	}
	if(*type == LINETYPE_TEXT || *type == LINETYPE_END){
		if((line_size == 1 && *(*line) == '\n') || (line_size == 2 && *(*line) == '\r' && *(*line+1) == '\n')){
			*type = LINETYPE_END;
			return 1;
		}
		if(!is_valid_subrip_text_line(line, line_size)){
			return 0;
		}
		int8_t null_term = (!result->length)? 1 : 0;
		result->text = (char *) realloc(result->text,(result->length*sizeof(char))+line_size+null_term);
		memset(result->text+result->length,0,line_size+null_term);
		memcpy(result->text+result->length-(1-null_term),*line,line_size);
		result->length += line_size*(sizeof(char)+null_term);
		*type = LINETYPE_TEXT;
		return 1;
	}
}
int16_t get_subrip_part_output_length(subrip_part *item){
	if(!item){
		return 0;
	}
	int16_t index_size = (int16_t) log10(item->index) + 1;
	return (int) index_size + 1 + 2 + 	    //index = log10 + 1 + \r\n
		31 + 2 + 			    //timestamps + \r\n
		item->length +		    	    //text length
		2;				    //empty line for part end
}
int write_subrip_to_string(subrip_part *item, char **text){
	if(!text || !*text || !item){
		return 0;
	}
	sprintf(*text,"%d\r\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\r\n%s\r\n",item->index,item->start_time.hours,
										    item->start_time.minutes,item->start_time.seconds,
										    item->start_time.milliseconds, item->end_time.hours,
										    item->end_time.minutes, item->end_time.seconds,
										    item->end_time.milliseconds, item->text);
}
int repair_time(subrip_time *time){
	if(time->milliseconds < 0){
		time->seconds--;
		time->milliseconds += 1000;
	}
	if(time->milliseconds >= 1000){
		time->seconds++;
		time->milliseconds -= 1000;
	}
	if(time->seconds < 0){
		time->minutes--;
		time->seconds += 60;
	}
	if(time->seconds >= 60){
		time->minutes++;
		time->seconds -= 60;
	}
	if(time->minutes < 0){
		time->hours--;
		time->minutes += 60;
	}
	if(time->minutes >= 60){
		time->hours++;
		time->minutes -= 60;
	}
	if(time->hours > 99 && time->hours < 0){ //not allowed by subrip
		return 0;
	}
	return 1;
}
int move_time(subrip_part *item, int32_t move){
	const int32_t h_div = 3600000; //1000*60*60
	const int32_t m_div = 60000; // 1000*60
	const int16_t s_div = 1000;

	int8_t sign = (move>0)? 1 : -1;
	int32_t abs = move * sign;

	//dont use divisions, performance!
	while(abs > 0){
		if(abs >= h_div){
			abs -= h_div;
			item->start_time.hours += sign;
			item->end_time.hours += sign;
			continue;
		}
		if(abs >= m_div){
			abs -= m_div;
			item->start_time.minutes += sign;
			item->end_time.minutes += sign;
			continue;
		}
		if(abs >= s_div){
			abs -= s_div;
			item->start_time.seconds += sign;
			item->end_time.seconds += sign;
			continue;
		}
		item->start_time.milliseconds += (abs*sign);
		item->end_time.milliseconds += (abs*sign);
		abs = 0;
	}
	if(!repair_time(&(item->start_time)) || !repair_time(&(item->end_time))){
		return 0;
	}

	return 1;
}
int32_t check_time_parameter(char **arg){
	uint8_t size = 0;
	char *tmp = *arg;
	for(; *tmp != '\0';tmp++){
		if(size > 9){
			//size exceeded
			return 0;
		}
		size++;
		if(*tmp < 48 || *tmp > 57){
			return 0;
		}
	}
	int32_t ret = atoi(*arg);
	if(ret <= 0 || ret > 356400000){
		return 0;
	}
	return ret;
}
int main(int argc, char **argv){
	FILE *fi = NULL;
	FILE *fo = NULL;
	int32_t add_time = 0;
	int32_t sub_time = 0;
	int16_t errno = 0;
	int16_t c = 0;
	while((c = getopt(argc, argv, "i:o:a:s:h")) != -1){
		switch(c){
			case 'i':
				fi = fopen(optarg,"r");
				if(!fi){
					printf("error while opening input file!\n");
					return 1;
				}
				break;
			case 'o':
				fo = fopen(optarg,"w");
				if(!fo){
					printf("error while opening output file!\n");
					return 1;
				}
				break;
			case 'a':
				add_time = check_time_parameter(&optarg);
				if(!add_time || sub_time){
					usage();
					return 1;			
				}
				break;
			case 's':
				sub_time = check_time_parameter(&optarg);
				if(!sub_time || add_time){
					usage();
					return 1;			
				}
				break;
			case 'h':
				usage();
				return 0;
      			default:
				printf("illegal parameter!\n");
				usage();
				return 1;
		}

	}
	if(!fi || !fo){
		usage();
		return 1;
	}
	
	int16_t read = 0;
	char *line = NULL;
	ssize_t len = 0;
	uint32_t line_cnt = 0;
	subrip_part item = {};
	line_type curr_line = LINETYPE_INDEX;

	while((read = getline(&line, &len, fi)) != -1){
		line_cnt++;
		int8_t bom = (line_cnt == 1 && read > 3) && (*line == -17 && *(line+1) == -69 && *(line+2) == -65) ? 3 : 0; //"short-circuiting" prevents error
		if(bom){
			char *line_bom = line+3;
			if(!read_subrip_from_line(&line_bom, read-bom, &item, &curr_line)){
				printf("error while reading line %d\n",line_cnt);
				break;
			}
		} else {
			if(!read_subrip_from_line(&line, read, &item, &curr_line)){
				printf("error while reading line %d\n",line_cnt);
				break;
			}
		}
		if(curr_line == LINETYPE_END){
			//process shift, 
			if(!move_time(&item, add_time-sub_time)){
				printf("error in time processing!\n");
				break;
			}
			//write to file, 
			int16_t text_size = get_subrip_part_output_length(&item);
			char *text = malloc(text_size);
			if(!write_subrip_to_string(&item, &text)){
				printf("error while converting to string!\n");
				break;
			}
			fputs(text,fo);
			if(ferror(fo)){
				printf("error while writing to file!\n");
				break;
			}
			//cleanup
			curr_line = LINETYPE_INDEX;
			memset(text,0,text_size);
			free(text);
			memset(&item,0,sizeof(subrip_part));
			continue;
		}
		curr_line++;
	}
	fclose(fi);
	fclose(fo);
	return 0;
}
