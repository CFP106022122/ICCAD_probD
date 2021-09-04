#ifndef TR_GRAPH_FILE_H_
#define TR_GRAPH_FILE_H_

#include <iostream>
#include <stack>
#include "../graph.h"
using namespace std;

// class TR_EDGE{
// 	public:
// 		int start;
// 		int degree;
// 		TR_EDGE(){
// 			start = 0;
// 			degree = 0;
// 		}
// 		~TR_EDGE(){
// 			delete &start;
// 			delete &degree;
// 		}
// };
struct TR_EDGE{
	int start;
	int degree;
};
// class EDGEINF{
// 	public:
// 		int node;
// 		int id;
// 		EDGEINF(){
// 			node = 0;
// 			id = 0;
// 		}
// 		~EDGEINF(){
// 			delete &node;
// 			delete &id;
// 		}
// };
//��Ӧ�ڽӱ��бߵ���Ϣ���й�����㣬�Լ��ߵ�id
struct EDGEINF{
	int node;
	int id;
};

TR_EDGE *outEdge, *inEdge;
//TR_EDGE * outEdge, *inEdge;
EDGEINF *tredge, *edgeR;
// EDGEINF * tredge, *edgeR;
int vertexNum, edgeNum;
int * zeroInDegree;
int * inDegree;
int * topOrder;
//����ֻ��Ҫ���������������򣬲���Ҫ����Ϊ0�Ľ����Ϣ

stack<int> simpleStack;


void initGraph();
void readGraph(Graph& G);
void reverse();
void zeroVertex();//ֻͳ�����Ϊ0�Ľ�㣬��ͳ�Ƴ���Ϊ0�Ľ��
void topology();
void destroyGraph();

void initGraph(){
	outEdge = new TR_EDGE[vertexNum + 10];
	inEdge = new TR_EDGE[vertexNum + 10];
	tredge = new EDGEINF[edgeNum + 10];
	edgeR = new EDGEINF[edgeNum + 10];
	zeroInDegree = new int[vertexNum + 1];
	inDegree = new int[vertexNum];
	topOrder = new int[vertexNum];
}

void readGraph(Graph& G){
	initGraph();
	for (int i = 0; i < vertexNum; i++){
		zeroInDegree[i] = 0;
		inDegree[i] = 0;
		topOrder[i] = 0;
	}
	zeroInDegree[vertexNum] = 0;
	for (int i = 0; i < vertexNum + 10; i++){
		outEdge[i].degree = 0;
		outEdge[i].start = 0;
		inEdge[i].degree = 0;
		inEdge[i].start = 0;
	}
	for (int i = 0; i < edgeNum + 10; i++){
		tredge[i].id = 0;
		tredge[i].node = 0;
		edgeR[i].id = 0;
		edgeR[i].node = 0;
	}	
	vector<edge>* edge_list = G.get_edge_list();
	int m = 0;
	for(int i = 0; i < vertexNum; i++){
		outEdge[i].start = m;
		outEdge[i].degree = edge_list[i].size();
		for(int j = 0; j < edge_list[i].size(); j++){
			inEdge[edge_list[i][j].to].degree++;
			tredge[m].node = edge_list[i][j].to;
			tredge[m].id = m++;
		}
	}
	reverse();
	zeroVertex();
}

//��ʼ����߱�ʱ���������߱������߱��Ŀ�ʼ������ĸ�����ô��߱���Ӧ�ô洢�ĸ����
//�ߵ�ID�洢��Ӧ���Ǹ�ID���ɣ�<u,v>�����u�ĳ��ߣ����������v����ߣ���ʵ��һ����
void reverse(){
	int sum = 0, i;
	for (i = 0; i < vertexNum; i++){
		inEdge[i].start = sum;//ȷ��ÿ���������ڶ�Ӧ�����еĿ�ʼλ��
		sum += inEdge[i].degree;
	}
	int start, end, curEdge;
	for (i = 0; i < vertexNum; i++){
		start = outEdge[i].start;
		end = start + outEdge[i].degree;
		for (; start < end; start++){
			curEdge = tredge[start].node;
			edgeR[inEdge[curEdge].start].node = i;
			edgeR[inEdge[curEdge].start++].id = tredge[start].id;//ΪʲôҪ��start++������degree++�أ�
		}
	}
	sum = 0;
	for (i = 0; i < vertexNum; i++){
		inEdge[i].start = sum;//��Ϊ�ڳ�ʼ�������У�start��������������Ҫ�ָ�����ʼ��ֵ
		sum += inEdge[i].degree;
	}
}

void zeroVertex(){
	int zeroIn = 0;
	for (int i = 0; i < vertexNum; i++){
		if (0 == inEdge[i].degree){
			zeroInDegree[++zeroIn] = i;
		}
	}
	zeroInDegree[0] = zeroIn;
}

void topology(){
	int zero = zeroInDegree[0];
	int start, end;
	int cur, next;
	int topoNumber = 0;
	simpleStack = stack<int>();
	for (int i = 0; i < vertexNum; i++){
		inDegree[i] = inEdge[i].degree;
		inEdge[i].degree = 0;

	}

	for (int i = 1; i <= zero; i++){
		cur = zeroInDegree[i];
		simpleStack.push(cur);
		while (!simpleStack.empty()){
			cur = simpleStack.top();
			simpleStack.pop();
			topOrder[topoNumber] = cur;//��¼���˺Ŷ�Ӧ�Ľ��
			topoNumber++;
			//���������ݳ��߸�����߱���˳��
			start = outEdge[cur].start;
			end = start + outEdge[cur].degree;//��ǰ�����cur�����Ӧ�ĳ��߽��Ϊedge[start]
			while (start < end){
				int pCur = tredge[start].node;
				edgeR[inEdge[pCur].start + inEdge[pCur].degree].node = cur;
				edgeR[inEdge[pCur].start + inEdge[pCur].degree++].id = tredge[start].id;
				start++;
			}

			start = outEdge[cur].start;
			end = start + outEdge[cur].degree;
			while (start < end){
				next = tredge[--end].node;
				if (0 == --inDegree[next]){
					simpleStack.push(next);
				}
			}
		}
	}

	//������򣬲���������߱��󣬸��³��߱�
	for (int i = 0; i < vertexNum; i++){
		outEdge[i].degree = 0;
	}
	for (int i = 0; i < vertexNum; i++){
		cur = topOrder[i];//ȡ��˳��Ϊi�Ľ��ֵ
		start = inEdge[cur].start;
		end = start + inEdge[cur].degree;
		while (start < end){
			int pCur = edgeR[start].node;
			tredge[outEdge[pCur].start + outEdge[pCur].degree].node = cur;
			tredge[outEdge[pCur].start + outEdge[pCur].degree++].id = edgeR[start].id;
			start++;
		}
	}
}

void destroyGraph(){
	delete outEdge;
	delete inEdge;
	delete tredge;
	delete edgeR;
	delete zeroInDegree;
	delete inDegree;
	delete topOrder;
}

#endif
