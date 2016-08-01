#pragma once  
#include"HuffmanTree.h"  
#include<algorithm>  
#include<windows.h>  
#include<string.h>  
using namespace std;

typedef long long Longtype;//为了扩大其范围，int型能处理的范围已经不能满足，所以定义Long Long型予以表示  

struct CharInfo
{
	unsigned char _ch;//这里必须为unsigned，否则会造成截断，所以从-128~127调至0~255.  
	Longtype _count;
	string _code;

	CharInfo(unsigned char ch = 0)
		:_ch(ch)
		, _count(0)
	{}

	CharInfo operator+(CharInfo& file)
	{
		CharInfo tmp;
		tmp._count = this->_count + file._count;
		return tmp;
	}

	bool operator < (CharInfo& file)
	{
		return this->_count < file._count;
	}

	bool operator != (const CharInfo& file)const
	{
		return this->_count != file._count;
	}
};


template<class T>
class FileCompress
{
public:
	FileCompress()
	{
		for (int i = 0; i < 256; ++i)
		{
			_arr[i]._ch = i;
		}
	}

public:

	bool Compress(const char* filename)
	{
		//1.打开文件，统计文件字符出现的次数    
		long long Charcount = 0;
		assert(filename);
		FILE* fOut = fopen(filename, "rb");//"rb"为以二进制方式读取文件，这里的b就是binary。"wb"为以二进制方式写入文件  
		assert(fOut);					//以二进制和文本打开方式区别在于：以文本打开方式会将\r\n
										//转换为\n,二进制这不会有这样的转换
		char ch = fgetc(fOut);

		while (ch != EOF)
		{
			_arr[(unsigned char)ch]._count++;
			ch = fgetc(fOut);
			Charcount++;
		}

		//2.生成对应的huffman编码    
		GenerateHuffmanCode();

		//3.文件压缩    
		string compressFile = filename;
		compressFile += ".compress";
		FILE* fwCompress = fopen(compressFile.c_str(), "wb");
		assert(fwCompress);

		fseek(fOut, 0, SEEK_SET);
		ch = fgetc(fOut);
		char inch = 0;
		int index = 0;
		while (!feof(fOut))
		{
			string& code = _arr[(unsigned char)ch]._code;
			for (size_t i = 0; i < code.size(); ++i)
			{
				inch = inch << 1;
				if (code[i] == '1')
				{
					inch |= 1;
				}
				if (++index == 8)//对于形成的长串字符编码的切割，每8个bit为一个字节，便于读取  
				{
					fputc(inch, fwCompress);
					inch = 0;
					index = 0;
				}
			}
			ch = fgetc(fOut);
		}

		if (index)//考虑到可能会有切割完，剩余的字符码不够填充8个bit位的情况  
		{
			inch = inch << (8 - index);
			fputc(inch, fwCompress);
		}

		//4.配置文件，方便后续的解压缩；  
		string configFile = filename;
		configFile += ".config";
		FILE *fconfig = fopen(configFile.c_str(), "wb");
		assert(fconfig);

		char CountStr[128];
		_itoa(Charcount >> 32, CountStr, 10);
		fputs(CountStr, fconfig);
		fputc('\n', fconfig);
		_itoa(Charcount & 0xffffffff, CountStr, 10);
		fputs(CountStr, fconfig);
		fputc('\n', fconfig);

		CharInfo invalid;
		for (int i = 0; i < 256; i++)
		{
			if (_arr[i] != invalid)
			{
				fputc(_arr[i]._ch, fconfig);
				fputc(',', fconfig);
				fputc(_arr[i]._count + '0', fconfig);
				fputc('\n', fconfig);
			}
		}

		fclose(fOut);
		fclose(fwCompress);
		fclose(fconfig);

		return true;
	}
	//文件的解压  
	bool UnCompresss(const char* filename)
	{
		string configfile = filename;
		configfile += ".config";
		FILE* outConfig = fopen(configfile.c_str(), "rb");
		assert(outConfig);
		char ch;
		long long Charcount = 0;
		string line = ReadLine(outConfig);
		Charcount = atoi(line.c_str());
		Charcount <<= 32;
		line.clear();
		line = ReadLine(outConfig);
		Charcount += atoi(line.c_str());
		line.clear();

		while (feof(outConfig))
//feof()遇到文件结束，函数值为非零值，否则为0。当把数据以二进制的形式进行存放时，可能会有-1值的出现，
//所以此时无法利用-1值（EOF）做为eof()函数判断二进制文件结束的标志。  
		{
			line = ReadLine(outConfig);
			if (!line.empty())
			{
				ch = line[0];
				_arr[(unsigned char)ch]._count += atoi(line.substr(2).c_str());
				line.clear();
			}
			else
			{
				line = '\n';
			}
		}

		HuffmanTree<CharInfo> ht;
		CharInfo invalid;
		ht.CreatTree(_arr, 256, invalid);//重新建树

		HuffmanTreeNode<CharInfo>* root = ht.GetRootNode();

		string  UnCompressFile = filename;
		UnCompressFile += ".uncompress";
		FILE* fOut = fopen(UnCompressFile.c_str(), "wb");

		string CompressFile = filename;
		CompressFile += ".compress";
		FILE* fIn = fopen(CompressFile.c_str(), "rb");

		int pos = 8;
		HuffmanTreeNode<CharInfo>* cur = root;
		ch = fgetc(fIn);

		while ((unsigned char)ch != EOF)
		{
			--pos;
			if ((unsigned char)ch &(1 << pos))
			{
				cur = cur->_right;
			}
			else
			{
				cur = cur->_left;
			}
			if (cur->_left == NULL && cur->_right == NULL)
			{
				fputc(cur->_weight._ch, fOut);
				cur = root;
				Charcount--;
			}
			if (pos == 0)
			{
				ch = fgetc(fIn);
				pos = 8;
			}
			if (Charcount == 0)
			{
				break;
			}
		}

		fclose(outConfig);
		fclose(fIn);
		fclose(fOut);
		return true;
	}

protected:
	string ReadLine(FILE* fConfig)
	{
		char ch = fgetc(fConfig);
		if (ch == EOF)
		{
			return "";
		}
		string line;
		while (ch != '\n' && ch != EOF)
		{
			line += ch;
			ch = fgetc(fConfig);
		}
		return line;
	}

	void GenerateHuffmanCode()
	{
		HuffmanTree<CharInfo> hft;
		CharInfo invalid;

		hft.CreatTree(_arr, 256, invalid);
		_GenerateHuffmanCode(hft.GetRootNode());
	}

	void _GenerateHuffmanCode(HuffmanTreeNode<CharInfo>* root)
	{
		if (root == NULL)
		{
			return;
		}

		_GenerateHuffmanCode(root->_left);
		_GenerateHuffmanCode(root->_right);

		if (root->_left == NULL && root->_right == NULL)
		{
			HuffmanTreeNode<CharInfo>* cur = root;
			HuffmanTreeNode<CharInfo>* parent = cur->_parent;
			string& code = _arr[cur->_weight._ch]._code;

			while (parent)
			{
				if (parent->_left == cur)
				{
					code += '0';
				}
				else if (parent->_right == cur)
				{
					code += '1';
				}
				cur = parent;
				parent = cur->_parent;
			}

			reverse(code.begin(), code.end());
		}
	}

private:
	CharInfo _arr[256];
};

void TestFileCompress()
{

	FileCompress<CharInfo> fc;
	cout << "Input文件压缩中...." << endl;
	cout << "压缩用时： ";
	int begin1 = GetTickCount();
	fc.Compress("Input");//  
	int end1 = GetTickCount();//  
	cout << end1 - begin1 << endl << endl;
	
	cout << "Input文件解压中...." << endl;;
	cout << "解压用时： ";
	int begin2 = GetTickCount();
	fc.UnCompresss("Input");
	int end2 = GetTickCount();//用以测试解压用时  
	cout << end2 - begin2 << endl << endl;

	FileCompress<CharInfo> fc1;
	
	cout << "Input.BIG文件压缩中...." << endl;
	cout << "压缩用时： ";
	int begin3 = GetTickCount();
	fc1.Compress("Input.BIG");//  
	int end3 = GetTickCount();//  
	cout << end3 - begin3 << endl << endl;

	cout << "Input.BIG文件解压中...." << endl;
	cout << "解压用时： ";
	int begin4 = GetTickCount();
	fc1.UnCompresss("Input.BIG");
	int end4 = GetTickCount();  
	cout << end4 - begin4 << endl;
}

