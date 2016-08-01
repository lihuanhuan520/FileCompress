#pragma once  
#include"HuffmanTree.h"  
#include<algorithm>  
#include<windows.h>  
#include<string.h>  
using namespace std;

typedef long long Longtype;//Ϊ�������䷶Χ��int���ܴ���ķ�Χ�Ѿ��������㣬���Զ���Long Long�����Ա�ʾ  

struct CharInfo
{
	unsigned char _ch;//�������Ϊunsigned���������ɽضϣ����Դ�-128~127����0~255.  
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
		//1.���ļ���ͳ���ļ��ַ����ֵĴ���    
		long long Charcount = 0;
		assert(filename);
		FILE* fOut = fopen(filename, "rb");//"rb"Ϊ�Զ����Ʒ�ʽ��ȡ�ļ��������b����binary��"wb"Ϊ�Զ����Ʒ�ʽд���ļ�  
		assert(fOut);					//�Զ����ƺ��ı��򿪷�ʽ�������ڣ����ı��򿪷�ʽ�Ὣ\r\n
										//ת��Ϊ\n,�������ⲻ����������ת��
		char ch = fgetc(fOut);

		while (ch != EOF)
		{
			_arr[(unsigned char)ch]._count++;
			ch = fgetc(fOut);
			Charcount++;
		}

		//2.���ɶ�Ӧ��huffman����    
		GenerateHuffmanCode();

		//3.�ļ�ѹ��    
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
				if (++index == 8)//�����γɵĳ����ַ�������иÿ8��bitΪһ���ֽڣ����ڶ�ȡ  
				{
					fputc(inch, fwCompress);
					inch = 0;
					index = 0;
				}
			}
			ch = fgetc(fOut);
		}

		if (index)//���ǵ����ܻ����и��꣬ʣ����ַ��벻�����8��bitλ�����  
		{
			inch = inch << (8 - index);
			fputc(inch, fwCompress);
		}

		//4.�����ļ�����������Ľ�ѹ����  
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
	//�ļ��Ľ�ѹ  
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
//feof()�����ļ�����������ֵΪ����ֵ������Ϊ0�����������Զ����Ƶ���ʽ���д��ʱ�����ܻ���-1ֵ�ĳ��֣�
//���Դ�ʱ�޷�����-1ֵ��EOF����Ϊeof()�����ж϶������ļ������ı�־��  
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
		ht.CreatTree(_arr, 256, invalid);//���½���

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
	cout << "Input�ļ�ѹ����...." << endl;
	cout << "ѹ����ʱ�� ";
	int begin1 = GetTickCount();
	fc.Compress("Input");//  
	int end1 = GetTickCount();//  
	cout << end1 - begin1 << endl << endl;
	
	cout << "Input�ļ���ѹ��...." << endl;;
	cout << "��ѹ��ʱ�� ";
	int begin2 = GetTickCount();
	fc.UnCompresss("Input");
	int end2 = GetTickCount();//���Բ��Խ�ѹ��ʱ  
	cout << end2 - begin2 << endl << endl;

	FileCompress<CharInfo> fc1;
	
	cout << "Input.BIG�ļ�ѹ����...." << endl;
	cout << "ѹ����ʱ�� ";
	int begin3 = GetTickCount();
	fc1.Compress("Input.BIG");//  
	int end3 = GetTickCount();//  
	cout << end3 - begin3 << endl << endl;

	cout << "Input.BIG�ļ���ѹ��...." << endl;
	cout << "��ѹ��ʱ�� ";
	int begin4 = GetTickCount();
	fc1.UnCompresss("Input.BIG");
	int end4 = GetTickCount();  
	cout << end4 - begin4 << endl;
}

