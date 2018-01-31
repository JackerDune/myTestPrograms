#include <stdio.h>
#include <memory.h>

#include "libxml/xmlmemory.h"
#include "libxml/parser.h"

static int32_t open_xml_file(xmlDocPtr *doc, char *xml_file_path)
{
	*doc = xmlParseFile((const char *)xml_file_path);

	if (NULL == *doc) {
		return -1;
	} else {
		return 0;
	}
}

static int32_t move_to_dst_node(xmlDocPtr *doc, xmlNodePtr *cur, char *dst)
{
	xmlNodePtr node = NULL;
	node = xmlDocGetRootElement(*doc);

	if (NULL == node) {
		goto err;
	}

	if (xmlStrcmp(node->name, (const xmlChar *) "root")) {
		goto err;
	}

	node = node->xmlChildrenNode;
	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)dst))) {
			*cur = node->xmlChildrenNode; /* skip to item */
			goto success; /* found at least one item */
		}
		node = node->next;
	}
success:
	return 0;
err:
	return -1;

}

xmlNodePtr move_to_child_node(xmlNodePtr *cur, char *dst)
{
	xmlNodePtr node = *cur;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)dst))) {
			return node->xmlChildrenNode; /* skip to item */
		}
		node = node->next;
	}
	
	return NULL;

}

xmlNodePtr get_child_node_by_content(xmlNodePtr *cur, char *NodeName, char *Content)
{
	xmlNodePtr node = *cur;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)NodeName))
			&& (!xmlStrcmp(Content, node->xmlChildrenNode->content))) {
			return node->xmlChildrenNode; /* skip to item */
		}
		node = node->next;
	}
	
	return NULL;

}

static xmlChar* get_content_by_name(xmlNodePtr *cur, char *name)
{
	xmlNodePtr node = *cur;
	xmlChar *value = NULL;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)name))) {
			value = xmlNodeGetContent(node);
			goto out;
		}
		node = node->next;
	}
out:
	return value;

}

static int get_child_node_by_name(xmlNodePtr *cur, xmlNodePtr *child, char *name)
{
	xmlNodePtr node = *cur;

	while (NULL != node) {
		if ((!xmlStrcmp(node->name, (const xmlChar *)name))) {
			*child = node->xmlChildrenNode;
			return 0;
		}
		node = node->next;
	}

	return -1;
}

static xmlChar* get_child_node_content_by_name(xmlNodePtr *cur, char *NodeName)
{
	xmlNodePtr child;
	
	child = move_to_child_node(cur, NodeName);
	if (child)
		return child->content;
	else
		return NULL;
}

static int32_t free_xml_item(int argc, ...)
{
	va_list ap;
	xmlChar *tmp = NULL;
	int i = 0;

	va_start(ap, argc);
	for (i = 0; i < argc; i++) {
		tmp = va_arg(ap, xmlChar *);
		if (tmp) {
			xmlFree(tmp);
		}
	}
	va_end(ap);

	return 0;
}

typedef enum {
	INIT_MODULE_ID_WEB,
	INIT_MODULE_ID_APP,
	INIT_MODULE_ID_DEVICE,
	INIT_MODULE_ID_CONNTRACK,
	INIT_MODULE_MAX
}INIT_MODULE;

#define MODULE_NAME_LEN 32
#define MODULE_MAX_OBJECT_NUM 16

//INIT_MODULE_WEB
typedef enum {
	WEB_MODULE_OBJ_WEBUI,
	WEB_MODULE_OBJ_MAX = MODULE_MAX_OBJECT_NUM,
}WEB_MODULE_OBJECT;

//INIT_MODULE_ID_APP
typedef enum {
	APP_MODULE_OBJ_AV,
	APP_MODULE_OBJ_IPS,
	APP_MODULE_OBJ_SAAE,
	APP_MODULE_OBJ_MAX = MODULE_MAX_OBJECT_NUM,
}APP_MODULE_OBJECT;

//INIT_MODULE_ID_DEVICE
typedef enum {
	DEV_MODULE_OBJ_DEVICE,
	DEV_MODULE_OBJ_MAX = MODULE_MAX_OBJECT_NUM,
}DEVICE_MODULE_OBJECT;


//INIT_MODULE_ID_CONNTRACK
typedef enum {
	CT_MODULE_OBJ_HASHNUM,
	CT_MODULE_OBJ_RESERVE_CT,
	CT_MODULE_OBJ_MAX = MODULE_MAX_OBJECT_NUM
}CONNTRACK_MODULE_OBJECT;

enum {
	HW_PLATFORM_LOW,
	HW_PLATFORM_MIDDLE,
	HW_PLATFORM_HIGH,
	VM_PLATFORM,
	PLATFORM_MAX,
};

struct ElementInfo {
	char ElementInfoInCfgFile[32];
	int ElementValue;
};

struct ObjectInfo {
	char ObjectInfoInCfgFile[32];
	int ObjectValue[PLATFORM_MAX];
};

struct dpModuleObject {
	char ObjectName[MODULE_NAME_LEN];
	struct ElementInfo ObjectFunction;
	struct ObjectInfo ObjectValue;
};

struct dpInitModule {
	char ModuleName[MODULE_NAME_LEN];
	struct ElementInfo ModuleFunction;
	struct dpModuleObject dpModuleObj[MODULE_MAX_OBJECT_NUM];
};

enum {
	FUNCTION_OFF,
	FUNCTION_ON
};


#define MODULE_INIT_BEGIN(modid, modname, modfuncvalue) \
	[modid] = \
	{\
		.ModuleName = (modname),\
		.ModuleFunction = \
		{\
			.ElementInfoInCfgFile = "Function",\
			.ElementValue = modfuncvalue,\
		}

#define OBJECT_INIT(objid, objname, objfuncvalue, objlowvalue, objmidvalue, objhighvalue, objvmvalue) \
		.dpModuleObj[objid] = \
		{ \
			.ObjectName = (objname), \
			.ObjectFunction = {\
				.ElementInfoInCfgFile = "Function",\
				.ElementValue = objfuncvalue,\
			},\
			.ObjectValue= { \
				.ObjectInfoInCfgFile= "value",\
			    .ObjectValue[HW_PLATFORM_LOW] = objlowvalue,\
			    .ObjectValue[HW_PLATFORM_MIDDLE] = objmidvalue,\
			    .ObjectValue[HW_PLATFORM_HIGH] = objhighvalue,\
			    .ObjectValue[VM_PLATFORM] = objvmvalue,\
			},\
		}

#define MODULE_INIT_END(modid)\
	}


struct dpInitModule g_dp_init_module[INIT_MODULE_MAX] = 
{
	//WEBUI MODULE
	MODULE_INIT_BEGIN(INIT_MODULE_ID_WEB, "WebModule", FUNCTION_ON),
		OBJECT_INIT(WEB_MODULE_OBJ_WEBUI, "WebObjWEBUI", FUNCTION_ON, 100, 100, 100, 100),
	MODULE_INIT_END(INIT_MODULE_ID_WEB),

	//APP MODULE
	MODULE_INIT_BEGIN(INIT_MODULE_ID_APP, "AppModule", FUNCTION_ON),
		OBJECT_INIT(APP_MODULE_OBJ_AV, "AppObjAV", FUNCTION_ON, 100, 100, 100, 100),
		OBJECT_INIT(APP_MODULE_OBJ_IPS, "AppObjIPS", FUNCTION_ON, 100, 100, 100, 100),
		OBJECT_INIT(APP_MODULE_OBJ_SAAE, "AppObjSAAE", FUNCTION_ON, 100, 100, 100, 100),
	MODULE_INIT_END(INIT_MODULE_ID_APP),
	
	//DEVICE MODULE
	MODULE_INIT_BEGIN(INIT_MODULE_ID_DEVICE, "DeviceModule", FUNCTION_ON),
		OBJECT_INIT(DEV_MODULE_OBJ_DEVICE, "DeviceObjDev", FUNCTION_ON, 1024, 2048, 4096, 1024),
	MODULE_INIT_END(INIT_MODULE_ID_DEVICE),

	//CONNECT TRACK MODULE
	MODULE_INIT_BEGIN(INIT_MODULE_ID_CONNTRACK, "ConntrackModule", FUNCTION_ON),
		OBJECT_INIT(CT_MODULE_OBJ_HASHNUM, "ConntrackObjHashSpec", FUNCTION_ON, 40960, 81920, 102400, 102400),
		OBJECT_INIT(CT_MODULE_OBJ_RESERVE_CT, "ConntrackObjRsvCT", FUNCTION_ON, 40960, 81920, 102400, 102400),
	MODULE_INIT_END( INIT_MODULE_ID_CONNTRACK)
};

#define IS_OBJ_SPEC_VALUE_VALID(value) ((value >= 1024) && (value <= 204800))

#if 0
struct dpInitModule g_dp_init_module[INIT_MODULE_MAX] = {
	[INIT_MODULE_ID_APP] = 
	{
		.ModuleName = "AppModule",
		.ModuleSwitch = SWITCH_ON,
		.dpModuleObj[APP_MODULE_OBJ_AV] = 
		{
			.ObjectName = "AppObjAV",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 0,
		},
		.dpModuleObj[APP_MODULE_OBJ_IPS] = 
		{
			.ObjectName = "AppObjIPS",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 0,
		},
		.dpModuleObj[APP_MODULE_OBJ_SAAE] = 
		{
			.ObjectName = "AppObjSAAE",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 0,
		},
	},
	[INIT_MODULE_ID_DEVICE] = 
	{
		.ModuleName = "DeviceModule",
		.ModuleSwitch = SWITCH_ON,
		.dpModuleObj[DEV_MODULE_OBJ_DEVICE] = 
		{
			.ObjectName = "DeviceObjDev",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 1024,
		},
	},
	[INIT_MODULE_ID_CONNTRACK] = 
	{
		.ModuleName = "ConntrackModule",
		.ModuleSwitch = SWITCH_ON,
		.dpModuleObj[CT_MODULE_OBJ_STATISTIC] = 
		{
			.ObjectName = "ConntrackObjStatis",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 10,
		},
		.dpModuleObj[CT_MODULE_OBJ_EXCEPTION_CONN] = 
		{
			.ObjectName = "ConntrackObjExcon",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 10,
		},
		.dpModuleObj[CT_MODULE_OBJ_NAT] = 
		{
			.ObjectName = "ConntrackObjNat",
			.ObjectSwitch = SWITCH_ON,
			.ObjectValue = 10,
		},
	},	
};
#endif

int get_dp_module_id_by_module_name(char * appModuleName)
{
	int i = 0; 
	
	for (i = 0; i < INIT_MODULE_MAX; i++) {
		if (!strcmp(appModuleName, g_dp_init_module[i].ModuleName))
			return i;
	}	

	return -1;
}


int get_dp_object_id_by_obj_name(char * objName, int ModuleIndex)
{
	int i = 0; 
	
	for (i = 0; i < MODULE_MAX_OBJECT_NUM; i++) {
		if (!strcmp(objName, g_dp_init_module[ModuleIndex].dpModuleObj[i].ObjectName))
			return i;
	}	

	return -1;
}

void phase_spec_value_for_diff_platform(char *pvalue, int modid, int objid)
{
	char *seps = "|";
	char *tmpBuf = NULL;
	int i = 0;
	int tmpValue = 0;

	if (pvalue == NULL)
		return;

	tmpBuf = strtok(pvalue, seps);
	while (tmpBuf) {
		tmpValue = atoi(tmpBuf);
		if (IS_OBJ_SPEC_VALUE_VALID(tmpValue))
			g_dp_init_module[modid].dpModuleObj[objid].ObjectValue.ObjectValue[i] = tmpValue;
	
		i++;
		if (i >= PLATFORM_MAX)
			break;
		tmpBuf = strtok(NULL, seps);
	}

	return;
	
}


void phase_xml_file_and_init_module(xmlNodePtr *node, int moduleIndex)
{
	xmlNodePtr ModuleNode = *node;
	xmlChar *value;
	xmlNodePtr ObjectNode = *node; 
	xmlNodePtr childNode = NULL;
	int objId = 0;
	int i = 0;

	value = get_content_by_name(&ModuleNode, "Function");
	g_dp_init_module[moduleIndex].ModuleFunction.ElementValue = atoi(value);

	
	free_xml_item(1, value);
	
	for (; ObjectNode != NULL; ObjectNode = ObjectNode->next) {
		if (strcmp(ObjectNode->name, "Object"))
			continue;
			
		printf("Obj: %s\n", ObjectNode->name);
		childNode = ObjectNode->xmlChildrenNode;
		value = get_content_by_name(&childNode, "ObjectName");
		printf("value: %s\n", value);
		if (value)
			objId = get_dp_object_id_by_obj_name(value, moduleIndex); 
		free_xml_item(1, value);
		if (objId != -1) {
			value = get_content_by_name(&childNode, "Function");
			if (value)
				g_dp_init_module[moduleIndex].dpModuleObj[objId].ObjectFunction.ElementValue = atoi(value);
			
			free_xml_item(1, value);
			value = get_content_by_name(&childNode, "Value");
			if (value) {
				phase_spec_value_for_diff_platform((char *)value, moduleIndex, objId);	
			}
			free_xml_item(1, value);
		}
	}
}

int main(int argc, char **argv)
{
	int ret = 0;
	xmlDocPtr doc;
	xmlNodePtr ModuleNode;
	xmlNodePtr ModuleNameNode;
	xmlNodePtr Node;
	xmlChar *value;
	int ModuleIndex = -1;

	ret = open_xml_file(&doc, "dplane_config.xml");
	if (ret == -1) {
		printf("open xml file error!\n");
		return 0;
	}
#if 0
	ret = move_to_dst_node(&doc, &ModuleNode, "Module");	
	if (ret == -1) {
		printf("%s, %d\n", __FUNCTION__, __LINE__);
		return -1;
	}
#endif
	Node = xmlDocGetRootElement(doc);

	if (NULL == Node) {
		printf("%s, %d, error NULL point!\n", __FUNCTION__, __LINE__);
	}

	if (xmlStrcmp(Node->name, (const xmlChar *) "root")) {
		printf("%s, %d, error no root node!\n", __FUNCTION__, __LINE__);
	}

	Node = Node->xmlChildrenNode;
	while (NULL != Node) {
		if ((!xmlStrcmp(Node->name, "Module"))) {
			 ModuleNode = Node->xmlChildrenNode; /* skip to item */
			value = get_content_by_name(&ModuleNode, "ModuleName");
			printf("value: %s", value);
			ModuleIndex = get_dp_module_id_by_module_name(value); 
			if (ModuleIndex != -1)
				phase_xml_file_and_init_module(&ModuleNode, ModuleIndex);				
			
			free_xml_item(1, value);
		}
		Node = Node->next;
	}
	
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}
