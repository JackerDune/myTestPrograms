#ifndef __MAIN_H__
#define __MAIN_H__

#define XML_NAME_LEN 	64
#define TYPE_XML_INT    0
struct check_range_node_parameter
{
	char name[XML_NAME_LEN+1];
	int (*check_func)(void *ptr);
	int type;
};
// str is A-Z,0-9,a-z,@._-()[]|
int name_str_check(const char *name, int len)
{
	int check_index = 0;

	while(check_index <len)
	{
		if ((*(name + check_index)>= '!' && *(name + check_index)< '(')
			 || (*(name + check_index)>')' && *(name + check_index)<'-')
			 || (*(name + check_index) == '/') || (*(name + check_index) == '`')
			 || (*(name + check_index) > '9' && *(name + check_index)< '@')
			 || (*(name + check_index) == '\\')
			 || (*(name + check_index) > ']' && *(name + check_index)< '_')
			 || (*(name + check_index) =='\'')|| (*(name + check_index) == '{')
			 || (*(name + check_index) == '}')|| (*(name + check_index) == '~'))
			 return -1;
		check_index++;
	}
	return 0;
}

int ip_address_common_check(char *ip);
#define CHECK_XML_IP(module, name, type)  \
        static inline int module##_xml_check_range_##name(void *ptr)\
           {\
				   if (ip_address_common_check((char *)(((type*)ptr)->name)) < 0) \
						return 250;\
				   else \
						return 0;\
           }

/*
 *param @str: buffer to be checked
 *param @str_len:length of str
 *return value: 1 for success, 0 for failed;
 *
 *str的长度应该是页面规定最大值+1
 *如果str中没有汉字，则只允许str的长度为str_len的长度的一半，否则可以占满整个长度-1
 */
static inline int guish_check_string_is_legal(const char *str, unsigned int str_len)
{
	char c;
	int flag = 0;//有无汉字的标志位
	int num = 0;
	int len = strlen(str);
	while((c = *str++)){
			
		if((c & 0x80) && (*str & 0x80)){
			num++;
			str++;//如果是汉字，则计数+1后，将这两个字节跳过去
			flag = 1;			
		} else {
			if(name_str_check(&c, 1) < 0)//检测英文的特殊字符
				return 0;	
		}
		//如果不是汉字，则str不自加，将str赋予c，继续逐个检测
	}

	if(flag){
		if(len > ((str_len - 1) / 2 + num))//num为汉字的个数，有一个就多开放一个字节
			return 0;
	} else {
		if(len > (str_len - 1) / 2)
			return 0;
	}
	return 1;
} 

#define CHECK_XML_INT_INIT(module, name, type, min, max)\
        static inline int module##_xml_check_range_##name(void *ptr)\
           {\
		      if ((((type*)ptr)->name < (int)min) || ((((type*)ptr)->name > (int)max)))\
					return 250;\
			   else \
				   return 0;\
           }
#define CHECK_XML_LONG_INIT(module, name, type, min, max)\
        static inline int module##_xml_check_range_##name(void *ptr)\
           {\
		      if ((((type*)ptr)->name < (long)min) || ((((type*)ptr)->name > (long)max)))\
					return 250;\
			   else \
				   return 0;\
           }
#define CHECK_XML_ULONG_INIT(module, name, type, min, max)\
        static inline int module##_xml_check_range_##name(void *ptr)\
           {\
		      if ((((type*)ptr)->name < (unsigned long)min) || ((((type*)ptr)->name > (unsigned long)max)))\
					return 250;\
			   else \
				   return 0;\
           }
#define CHECK_XML_STRING_INIT(module, name, type, maxlen)\
        static inline int module##_xml_check_range_##name(void *ptr)\
           {\
		      if(guish_check_string_is_legal((const char *)((type*)ptr)->name) == 0)\
					return 250;\
			   else \
				   return 0;\
           }

#define HASH_RANGE_NODE_PARAM_ADD(module, name, type) \
         {#name, module##_xml_check_range_##name, type}
#endif
