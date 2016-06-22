/**
 * @file func_network_set_wifi_ccode.cpp
 * @brief 
 * @author Jianjiao Sun <jianjiaosun@163.com>
 * @version 1.0
 * @date 2015-08-10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * The data structure in nvm file, analysis the file data as below
 * |  label  | attribute | separator |   
 * | xxxxxx= |    xxx    |     ^@    |
 */

#define CC_TEST
//#define CC_TEST_PC

#define DEBUG       		printf

#define NVM_FILE			"bcm4350-ffff-ffff.nvm"	
#define COUNTRY_CODE_LABEL	"ccode="
#define REGREV_LABE			"regrev="
#define CCODE_LEN			8
#define REGRV_LEN			8
#define MAX_CONFIG_LEN		1024
#define SEPARATOR			0			/*ASCII of ^@*/

#define RECOVER_CMD_LEN		256

#ifndef CC_TEST_PC
#define DEFAULT_NVM_FILE	"/basic/firmware/bcm4350-ffff-ffff.nvm"
#define DEST_NVM_FILE		"/3rd_rw/brcm/bcm4350-ffff-ffff.nvm"
#define DEFAULT_PATH		"/basic/firmware"
#define DEST_PATH			"/3rd_rw/brcm"
#else
#define DEFAULT_NVM_FILE	"src.nvm"
#define DEST_NVM_FILE		"dest.nvm"
#define DEFAULT_PATH		"."
#define DEST_PATH			"./test"
#endif

enum COUNTRY_CODE_IDX {
	CC_US,
	CC_EU,
	CC_AU,
	CC_BR,
	CC_INVALID,
};

struct ccode_st {
	char code[CCODE_LEN];	
	char regrv[REGRV_LEN];
};

static struct ccode_st g_ccode[] = {{"US", "140"}, 
									{"EU", "90"},
									{"AU", "23"},
									{"BR", "14"},
								   }; 

static int fork_exec(const char *path, char *const argv[]);

/**
 * @brief Skip attribute of source file, as we write the new attribute
 *
 * @param fp_src		Source file Pointer
 *
 * @return 	0			Success
 			Others	    Failed
 */
static int skip_change_attr(FILE *fp_src)
{
	char ch;
	size_t nread;

	nread = fread(&ch, sizeof(char), 1, fp_src);
	while ((0 != (int) ch) && (!feof(fp_src)))
	{
		if (nread != 1)
		{
			DEBUG("%s, read error\n", __FUNCTION__);
			return -1;
		}

		nread = fread(&ch, sizeof(char), 1, fp_src);
	} 

	return 0;
}

/**
 * @brief Get attribute and separator from source nvm file
 *
 * @param fp_src		File pointer of source file
 * @param buf			Buffer for attribute 
 * @param len			Buffer length
 *
 * @return 
 */
static int get_attr_separator(FILE *fp_src, char *buf, size_t len)
{
	char ch;
	size_t nread;
	size_t inc;
	char *attr = buf;

	nread = fread(&ch, sizeof(char), 1, fp_src);
	for (inc = 0; ((0 != (int) ch) && (!feof(fp_src)) && (inc < len)); inc++, attr++)
	{
		if (nread != 1)
		{
			DEBUG("%s, read error\n", __FUNCTION__);
			return -1;
		}
		*attr = ch;

		nread = fread(&ch, sizeof(char), 1, fp_src);
	} 

	*attr = ch;

	return (attr - buf + 1);
}


/**
 * @brief Get attribute and separator from source nvm file
 *
 * @param fp_src		File pointer of source file
 * @param buf			Buffer for attribute 
 * @param len			Buffer length
 *
 * @return 
 */
static int copy_rest(FILE *fp_src, FILE *fp_dest)
{
	size_t nread;
	char buf[MAX_CONFIG_LEN];

	nread = fread(buf, sizeof(char), MAX_CONFIG_LEN, fp_src);
	while (!feof(fp_src))
	{
		if (nread != MAX_CONFIG_LEN)
		{
			DEBUG("%s, read error\n", __FUNCTION__);
			return -1;
		}

		if (nread != fwrite(buf, sizeof(char), nread, fp_dest))
		{
			DEBUG("%s, write failed\n", __FUNCTION__);
			return -1;
		}

		nread = fread(buf, sizeof(char), MAX_CONFIG_LEN, fp_src);
	} 

	if (nread != fwrite(buf, sizeof(char), nread, fp_dest))
	{
		DEBUG("%s, write failed\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

/**
 * @brief Read label from source file 
 * 	The last of file have 3 ^@
 * 	get label shuold return them, and copy them to destinate file
 *
 * @param fp		File pointer of source File
 * @param label		Label buffer
 * @param size		Length of label
 *
 * @return 			Length of Label
 					0 Is failed
 */
static int get_label(FILE *fp, char *label, size_t size)
{
	char ch;
	size_t nread;
	char *tmp;

	nread = fread(&ch, sizeof(char), 1, fp);
	for (tmp = label; (!feof(fp)) && (((size_t) (tmp - label)) < size); tmp++)
	{
		if (nread != 1)
		{
			DEBUG("%s, read error\n", __FUNCTION__);
			return 0;
			break;
		}

		*tmp = ch;
		if ('=' == ch)
		{
			break;
		}

		nread = fread(&ch, sizeof(char), 1, fp);
	}

	return (tmp - label + 1);
}

/**
 * @brief Replace attribute
 *
 * @param fp_src	File pointer of source file
 * @param buf		Buffer for new attribute and separator
 * @param len		Length of buffer
 * @param attr		The new attribute
 *
 * @return 			0 			Success
 					others		failed
 */
static int new_attr(FILE *fp_src, char *buf, size_t len, const char *attr)
{
	int nr; 

	if (0 != skip_change_attr(fp_src))
	{
		return -1;
	}

	if (((nr = snprintf(buf, len, "%s%c", attr, SEPARATOR)) < 0) || ((size_t ) nr == len)) 
	{
		return -1;
	}

	return (strlen(attr) + 1);
}

/**
 * @brief Change the attribute to "@attr"by searching "@label"
 *
 * @param fp_src		File handler of source nvm file
 * @param fp_dest		File handler of Destination nvm file	
 * @param label			Label for attribute to be changed
 * @param attr			The wanted attribute
 *
 * @return 				0		success
 						-1 		failed
 */
static int change_attr(FILE *fp_src, FILE *fp_dest, const char *label, const char *attr)
{
	int ret = -1;
	char buf[MAX_CONFIG_LEN];
	size_t buf_len = sizeof(buf);
	char *rest_buf;
	size_t rest_len;
	int label_len;
	size_t attr_len;
	size_t write_len;

	while (!feof(fp_src))
	{
		buf[0] = '\0';
		label_len = get_label(fp_src, buf, sizeof(buf));
		if (label_len <= 0)
		{ 
			DEBUG("Get label failed\n");
			break;
		}

		rest_len = buf_len - label_len;	
		rest_buf = buf + label_len;

		if (!memcmp(label, buf, strlen(label)))
		{
			attr_len = new_attr(fp_src, rest_buf, rest_len, attr);
			ret = 0;
		}
		else
		{
			attr_len = get_attr_separator(fp_src, rest_buf, rest_len);
		}

		if (attr_len <= 0)
		{
			DEBUG("%s, get attribute failed\n", __FUNCTION__);
			break;
		}

		write_len = label_len + attr_len;

		/*Write one config to nvm files*/
		if (write_len != fwrite(buf, sizeof(char), write_len, fp_dest))
		{
			DEBUG("write error\n");
			return -1;
		}

		if (0 == ret)
		{
			break;
		}
	}

	return ret;
}

/**
 * @brief Recover to default nvm file
 */
static void recover2default(void)
{
	char exec_cmd[RECOVER_CMD_LEN];

	snprintf(exec_cmd, sizeof(exec_cmd), "/bin/cp %s/* %s", DEFAULT_PATH, DEST_PATH);

	system(exec_cmd);
}

static int _change_ccode_nvm(FILE *fp_src, FILE *fp_dest, int idx)
{
	int ret;
	ret = change_attr(fp_src, fp_dest, COUNTRY_CODE_LABEL, g_ccode[idx].code);
	if (0 != ret)
	{
		DEBUG("change code failed\n");
		return ret;
	}

	ret = change_attr(fp_src, fp_dest, REGREV_LABE, g_ccode[idx].regrv);
	if (0 != ret)
	{
		DEBUG("change regrev failed\n");

		return ret;
	}

	ret = copy_rest(fp_src, fp_dest);
	if (0 != ret)
	{
		DEBUG("change ccode failed\n");
	}

	return ret;
}


/**
 * @brief Change country code
 *
 * @param idx Index for country code
 *
 * @return  0 		success
 			-1		failed
 */
static int change_ccode_nvm(int idx)
{
	FILE *fp_src;
	FILE *fp_dest;
	int ret;

	if ((idx < CC_US) || (idx >= CC_INVALID))
	{
		return -EINVAL;
	}

	if((fp_src = fopen(DEFAULT_NVM_FILE, "r")) == NULL) 
	{
		DEBUG("Can't open %s, program will to exit.", DEFAULT_NVM_FILE); 
		return -1;
	}

	if((fp_dest = fopen(DEST_NVM_FILE, "w+")) == NULL) 
	{
		DEBUG("Can't open %s, program will to exit.", DEST_NVM_FILE);

		fclose(fp_src);
		return -1;
	}

	ret = _change_ccode_nvm(fp_src, fp_dest, idx);

	fclose(fp_src);
	fclose(fp_dest);

	return ret;
}

static int fork_exec(const char *path, char *const argv[])
{
	pid_t pid;
	int status;
	int w;

	pid = fork();
	if (pid < 0)
	{
		DEBUG("%s, fork failed\n", __FUNCTION__);
		return -1;
	}

	if (0 == pid)
	{
		return execv(path, argv);
	}
	else
	{
		do {
			w = waitpid(pid, &status, 0);		
			if (w == -1) {
				DEBUG("waitpid");
				exit(EXIT_FAILURE);
			}

			if (WIFEXITED(status)) {
				DEBUG("exited, status=%d\n", WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				DEBUG("killed by signal %d\n", WTERMSIG(status));
			} else if (WIFSTOPPED(status)) {
				DEBUG("stopped by signal %d\n", WSTOPSIG(status));
			} else if (WIFCONTINUED(status)) {
				DEBUG("continued\n");
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		exit(EXIT_SUCCESS);
	}
}

/**
 * @brief Make sure the country code is effective imediately
 *
 * @param path		Path of command
 * @param argv[]	Arguments
 * @param idx		Index
 *
 * @return			0			Success
 					!0 			Failed
 */
static int set_ccode_cmd(int idx)
{
	const char *argv[4];
	int ret;

	argv[0] = "wl";
	argv[1] = "country";
	argv[2] = g_ccode[idx].code;
	argv[3] = NULL;

	ret = fork_exec("/bin/wl", (char *const *) argv);
	if (0 != ret)
	{
		DEBUG("%s, set ccode failed\n", __FUNCTION__);
	}

	return ret;
}


/**
 * @brief Change country code
 *
 * @param idx Index for country code
 *
 * @return  0 		success
 			-1		failed
 */
int network_change_ccode(int idx)
{
	int ret;

	ret = change_ccode_nvm(idx);
	if (0 != ret)
	{
		recover2default();	
	}
	else
	{
		ret = set_ccode_cmd(idx);
	}

	return ret;
}

#ifdef CC_TEST
#define NEW_LIEN_ASCII		10

int main(int argc, char *argv[])
{
	int op;
	
	for(; ;)
	{
		printf("\nPlease input the selection\n"
			"1: Set country code\n"
			"2: Recover to default\n"
			"q: Exit\n");

		printf(">");
		fflush(stdin);
		op = getchar();
		if (NEW_LIEN_ASCII	== op)
		{
			op = getchar();
		}

		if ('q' == op)
		{
			break;
		}

		op = op - '0';
		if ((op < 1) || (op > 3))
		{
			printf("\nInvalid input number\n");
			continue;
		}

		switch (op) 
		{
			case 1:
				network_change_ccode(3);
				break;
			case 2:
				recover2default();
				break;
			default:
				break;

		}

	}

	return 0;
}
#endif
