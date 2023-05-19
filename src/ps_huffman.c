#include "ps_huffman.h"
#include "ps_log.h"

#include <stdlib.h>

#define MAX_TREE 100

static u8 *binary = NULL;
static u8 buffer;
static int iter = 0;
static int n = 0;

typedef struct huffman_node_t
{
	unsigned char c;
	int freq;
	struct huffman_node_t *left, *right;
} HuffmanNode;

static HuffmanNode *CreateHuffmanNode();
static int isLeaf(HuffmanNode *node);
static void fillBuffer();
static int readBoolean();
static unsigned char readChar();
static HuffmanNode *readDeocder();
static void Print(HuffmanNode *node, int arr[], int top);
static unsigned int readInt();
static int isLeaf(HuffmanNode *node);
static void cleanupTree(HuffmanNode *root);

static HuffmanNode *CreateHuffmanNode()
{
	HuffmanNode *node = (HuffmanNode *)malloc(sizeof(HuffmanNode));

	if (node == NULL)
	{
		printf("Cannot create huffman node\n");
		return NULL;
	}

	return node;
}

static void fillBuffer()
{
	buffer = binary[iter++];
	n = 8;
}

static int readBoolean()
{
	n--;
	int ret = ((buffer >> n) & 1);
	if (n == 0)
		fillBuffer();
	return ret;
}

static unsigned char readChar()
{
	if (n == 8)
	{
		char x = buffer;
		fillBuffer();
		return (x & 0xff);
	}

	int x = buffer;

	x <<= (8 - n);

	int oldN = n;

	fillBuffer();

	n = oldN;

	x |= (buffer >> n);

	return (x & 0xff);
}

static HuffmanNode *readDeocder()
{
	int bit = readBoolean();
	// printf("%d\n", bit);
	if (bit)
	{
		HuffmanNode *node = CreateHuffmanNode();
		node->c = readChar();
		node->freq = -1;
		node->left = node->right = NULL;
		return node;
	}
	else
	{
		HuffmanNode *node = CreateHuffmanNode();
		node->c = '-';
		node->freq = -1;
		node->left = readDeocder();
		node->right = readDeocder();
		return node;
	}
}

static void Print(HuffmanNode *node, int arr[], int top)
{
	if (isLeaf(node))
	{
		printf("%c | ", node->c);

		for (int i = 0; i < top; i++)
		{
			printf("%d", arr[i]);
		}

		printf("\n");

		return;
	}
	arr[top] = 0;
	Print(node->left, arr, top + 1);
	arr[top] = 1;
	Print(node->right, arr, top + 1);
}

static unsigned int readInt()
{
	unsigned int x = 0;

	for (int i = 0; i < 4; i++)
	{
		unsigned char c = readChar();

		x <<= 8;

		x |= c;
	}

	return x;
}

static int isLeaf(HuffmanNode *node)
{
	if (node->left == NULL && node->right == NULL)
	{
		return 1;
	}

	return 0;
}

static void cleanupTree(HuffmanNode *root)
{
	if (root)
	{
		if (isLeaf(root))
		{
			free(root);
			return;
		}
		else
		{
			cleanupTree(root->left);
			cleanupTree(root->right);
			free(root);
		}
	}
}

u8 *decompress(u8 *input, u32 compressSize, u32 *bufferSize)
{
	binary = input;

	fillBuffer();

	HuffmanNode *root = readDeocder();

	// int arr[MAX_TREE];
	// int top = 0;

	int length = readInt();

	u8 *bufferFile = (u8 *)malloc(length);

	if (bufferFile == NULL)
	{
		ERRORLOG("Cannot allocate buffer for decompressed file");
	}

	INFOLOG("decompressed length %d", length);

	int index = 0;

	for (int i = 0; i < length; i++)
	{
		HuffmanNode *node = root;
		while (!isLeaf(node))
		{
			int bit = readBoolean();
			if (bit)
				node = node->right;
			else
				node = node->left;
		}
		bufferFile[index] = node->c;
		index++;
	}

	cleanupTree(root);

	n = 0;
	buffer = 0;
	binary = NULL;
	iter = 0;

	*bufferSize = length;

	return bufferFile;
}