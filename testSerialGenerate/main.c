#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define MAC2STR(ch) (isdigit(ch) ? (ch) : toupper(ch + 4))
#define SUNYA_SERIAL_LEN 29

static char mgt_mac[32] = {};
static char convert_result_array[10][10] = {};
static int j = 0;
static char *num2str(unsigned int num, unsigned int len)
{
	char *convert_result;
	int result = 0, i =  3;

	if (len > 4 || num >= pow(36, len))
	  return "ZZZZ";

	if (j > 9)
	  j = 0;

	convert_result = convert_result_array[j];
	memset(convert_result, '0', 5);

	do
	{
		result = num % 36;
		num /= 36;
		convert_result[i--] = result < 10 ? '0' + result : 'A' + result - 10;
	}while (num >= 36);

	if (num)
	  convert_result[i] = num < 10 ? '0' + num : 'A' + num - 10;

	if (len == 4)
	  convert_result[4] = 0;
	else
	  for (i = 0; i < 4 - len; i++)
	  {
		  if (convert_result[0] == '0')
		  {
			  convert_result[0] = convert_result[1];
			  convert_result[1] = convert_result[2];
			  convert_result[2] = convert_result[3];
			  convert_result[3] = 0;
		  }
		  else
			break;
	  }

	j++;

	return convert_result;
}

static int calc_crc(char *input_str)
{
	char *p;
	char tmp_buf[32] = {};
	unsigned long long cal_crc = 12315;
	int i = 0;

	if(!input_str || strlen(input_str) != 24){
		return -1;
	}

	p = input_str;

	while (*p)
	{
		if (isdigit(*p))
		  cal_crc += cal_crc ^ (*p - '0' + 1);
		else if (isalpha(*p))
		  cal_crc += cal_crc ^ (toupper(*p) - 'A' + 11);
		else if (*p != '-') {
			return -1;
		}

		tmp_buf[i++] = *p;
		if (*(p+1) == '-' || *(p+1) == 0) {
			tmp_buf[i++] = *num2str(cal_crc % 36, 1);
			cal_crc += cal_crc ^ ((cal_crc % 36) + 1);
		}
		p++;
	}

	memcpy(input_str, tmp_buf, sizeof(tmp_buf));
	return 0;
}

static int serial_value_calc(char p, unsigned long long crc)
{
	int value;

	if(isdigit(p))
	  value = p - '0';
	else if (isalpha(p))
	  value = toupper(p) - 'A' + 10;
	else
	  return -1;

	return value != crc % 36;
}

/*
如果合法的MAC返回0 ，不合法返回-1
合法的MAC为xx:xx:xx:xx:xx:xx 或者xx-xx-xx-xx-xx-xx
src_mac经过处理后去掉冒号保存在addr中
*/
int is_mac_valid (unsigned char *addr, const char *src_mac)
{
  unsigned char *ptr = NULL;
  int num = 6;
  int total;
  char sep = ':';
  unsigned char mac[20];
  int i;

  for(i = 0; i < 20; i++)
  {
	 if((src_mac[i] == '-')||(src_mac[i] == ':'))
	 {
	 	sep = src_mac[i];
		break;
	 }
  }
  
  memset (mac, 0, 20);
  strncpy ((char *)mac, src_mac, 20);
  mac[strlen ((char *)mac)] = '\0';
  ptr = (unsigned char *) strrchr ((const char *)mac, sep);
  while (ptr)
    {
      *ptr = '\0';
      total = 0;

      if (!isdigit (*(ptr + 1)) && !isdigit (*(ptr + 2))
	  && !isalpha (*(ptr + 1)) && !isalpha (*(ptr + 2)))
	return -1;

      if ((*(ptr + 3) != '\0') || (*(ptr + 2) == '\0'))
	return -1;

      if (isalpha (*(ptr + 1)))
	{
	  if (tolower (*(ptr + 1)) > 'f')
	    return -1;
	  total += (tolower (*(ptr + 1)) - 87) * 16;
	}
      else
	total += (*(ptr + 1) - 48) * 16;

      if (isalpha (*(ptr + 2)))
	{
	  if (tolower (*(ptr + 2)) > 'f')
	    return -1;
	  total += tolower (*(ptr + 2)) - 87;
	}
      else
	total += *(ptr + 2) - 48;

      num--;
      *(addr + num) = total;

      ptr = (unsigned char *) strrchr ((char *)mac, sep);
    }

  if (!isdigit (*(mac + 1)) && !isdigit (*(mac)) && !isalpha (*(mac + 1))
      && !isalpha (*(mac)))
    return -1;

  if ((*(mac + 2) != '\0') && (*(mac + 1) == '\0'))
    return -1;

  total = 0;
  if (isalpha (*(mac)))
    {
      if (tolower (*(mac)) > 'f')
	return -1;
      total += (tolower (*(mac)) - 87) * 16;
    }
  else
    total += (*(mac) - 48) * 16;

  if (isalpha (*(mac + 1)))
    {
      if (tolower (*(mac + 1)) > 'f')
	return -1;
      total += tolower (*(mac + 1)) - 87;
    }
  else
    total += *(mac + 1) - 48;

  *(addr) = total;

  if (num == 1)
    return 0;
  else
    return -1;

}

/*
*	Get 00:11:22:33:44:55形式输出的MAC地址串
* 
*	Input:
*		mac    pointer to mac addr, should be a 6 byte char array
* 
*	Output:
*		str
*
*	Return :
*		void
*/
void get_str_from_macaddr( unsigned char * mac , char * str )
{
	sprintf( str, "%02x-%02x-%02x-%02x-%02x-%02x", mac[ 0 ], mac[ 1 ], mac[ 2 ], mac[ 3 ], mac[ 4 ], mac[ 5 ] );
	return ;
}

/********************************** 
检查输入的mac是否有效
      0表示有效，-1表示无效
**********************************/
int check_valid_mac_addr( char * mac_addr )
{
    int len;
    char one_hex;
    int i;

    len = strlen( mac_addr );

    for ( i = 0; i < len ; i++ )
    {
        one_hex = mac_addr[ i ];
        if ( !( ( ( one_hex >= '0' ) && ( one_hex <= '9' ) ) ||
                ( ( one_hex >= 'A' ) && ( one_hex <= 'F' ) ) ||
                ( ( one_hex >= 'a' ) && ( one_hex <= 'f' ) ) ||
                ( one_hex == '-' ) ) )
        {
            return -1;
        }
    }

    return 0;
}
#define MACLISTFILE   "./MacAddressList.txt"
#define MACSNSTOREFILE "./MacSN.csv"
static int sunya_serial_regenerate (const char *serno)
{
	char tmp_serial[32] = {0};
	u_int8_t hd[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
	FILE *fp = NULL; 
	char Buff[64] = {0};


	fp =fopen(MACLISTFILE, "r");
	if (fp == NULL) {
		printf("open %s failed!\n", MACLISTFILE);
		return -1;
	}
	
    while(fgets(Buff, 64, fp) != NULL) {
		if(is_mac_valid(hd, Buff) == 0) {
			/* generate new serial */
			memcpy(tmp_serial, serno, 4);
			memcpy(tmp_serial+4, serno+5, 5);
			memcpy(tmp_serial+9, serno+11, 5);


			/* store part of mgt's mac address */
			snprintf(mgt_mac, sizeof(mgt_mac) - 1, "%02X%02X%02X%02X",
						hd[2], hd[3], hd[4], hd[5]);
			tmp_serial[10] = '2';
			tmp_serial[14] = '-';
			tmp_serial[15] = MAC2STR(mgt_mac[0]);
			tmp_serial[16] = MAC2STR(mgt_mac[1]);
			tmp_serial[17] = MAC2STR(mgt_mac[2]);
			tmp_serial[18] = MAC2STR(mgt_mac[3]);
			tmp_serial[19] = '-';
			tmp_serial[20] = MAC2STR(mgt_mac[4]);
			tmp_serial[21] = MAC2STR(mgt_mac[5]);
			tmp_serial[22] = MAC2STR(mgt_mac[6]);
			tmp_serial[23] = MAC2STR(mgt_mac[7]);

			/* calc crc */
			calc_crc(tmp_serial);

			printf("mac address : %02x:%02x:%02x:%02x:%02x:%02x, serial number: %s\n", 
					hd[0],hd[1],hd[2],hd[3],hd[4],hd[5],tmp_serial);
		}
	}

	fclose(fp);
	return 0;
}

int sunya_serial_check(char *input_str)
{
	char *p;
	unsigned long long cal_crc = 12315;
	int i = 0;

	if(!input_str)
	  return -1;


	p = input_str;
	while ((p < input_str + SUNYA_SERIAL_LEN) && *p) {
		if (isdigit(*p)) {
			i++;
			if (!(i % 5) && serial_value_calc(*p, cal_crc))
			  return -1;

			cal_crc += cal_crc ^ (*p - '0' + 1);
		}
		else if (isalpha(*p)) {
			i++;
			if (!(i % 5) && serial_value_calc(*p, cal_crc))
			  return -1;

			cal_crc += cal_crc ^ (toupper(*p) - 'A' + 11);
		}
		else if (*p != '-' || (i % 5))
		  return -1;

		p++;
	}

	if(i != 25) {
		printf("invalid serial number !\n");
	   	return -1;

	}

	if (input_str[12] == '3') {
		printf("Generating serial number, please wait.\n");
		sunya_serial_regenerate(input_str);
	}else {
		printf("invalid serial number !\n");
	   	return -1;
	}
	
	return 0;
}


int main(int argc, char **argv)
{
	char serial[1024];
	int n  = 0;

	printf("please input general serial number!\n");
	n = scanf("%s", &serial);
	if (serial == NULL) {
		printf("Invalid Serial Number!\n");
		return -1;
	}

	sunya_serial_check(serial);		

	return 0;
}
