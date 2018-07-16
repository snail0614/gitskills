/*************************************************************************
	> File Name: mempool.h
	> Author: xkk
	> Mail: xukunkun0614@163.com 
	> Created Time: Fri 22 Jun 2018 02:40:24 PM PDT
 ************************************************************************/
#ifndef _MEMPOOLH_
#define _MEMPOOLH_
template<int ObjSize,int NumOfObj = 20>
class MemPool{
private:
	const int MemBlockSize;
	const int ItemSize;

	struct FreeNode{
		FreeNode* pNext;
		char obj_size[ObjSize];//存放Obj，相当于分配Obj内存，Obj用链表储存
	};

	struct MemBlock{
		MemBlock *pNext;
		FreeNode data[NumOfObj];
	};

	FreeNode* freeNodeHeader;
	MemBlock* memBlockHeader;

public:
	MemPool():ItemSize(ObjSize + sizeof(FreeNode *)),MemBlockSize(sizeof(MemBlock *) + NumOfObj * (ObjSize + sizeof(FreeNode *))){
		freeNodeHeader = NULL;
		memBlockHeader = NULL;
	}

	~MemPool(){
		MemBlock* ptr;
		while(memBlockHeader){
			ptr = memBlockHeader->pNext;
			delete memBlockHeader;
			memBlockHeader = ptr;
		}
	}
	void* malloc();
	void* free(void*);

};

//分配空闲的节点
template<int ObjSize,int NumofObjs> 
void* MemPool<ObjSize,NumofObjs>::malloc(){
    if(freeNodeHeader==NULL){      //无空闲节点
            MemBlock* newBlock = new MemBlock;
            newBlock->data[0].pNext = NULL; 
//设置内存块的第一个节点为空闲节点链表的最后一个,尾插法
            for(int i=1; i<NumofObjs;++i)
                newBlock->data[i].pNext = &newBlock->data[i-1];
            freeNodeHeader = &newBlock->data[NumofObjs-1];
            newBlock->pNext = memBlockHeader;
            memBlockHeader = newBlock->pNext;

    }
    //返回空节点闲链表的第一个节点
    void* freeNode=freeNodeHeader;
    freeNodeHeader=freeNodeHeader->pNext;
    return freeNode;
}

                                                                       //释放已经分配的节点
 template<int ObjSize,int NumofObjs> 
 void* MemPool<ObjSize,NumofObjs>::free(void* p){
     FreeNode* pNode=(FreeNode*)p;
     pNode->pNext=freeNodeHeader;//将释放的节点插入空闲节点头部
     freeNodeHeader=pNode;
 }
 /***end mempool.h***/

#endif
