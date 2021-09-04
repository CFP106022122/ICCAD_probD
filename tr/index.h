#ifndef INDEX_FILE_H_
#define INDEX_FILE_H_

#include "tr_graph.h"
#include "bfl.h"
#include "../graph.h"
#include <iostream>

using namespace std;

struct DEGREE{//���ﲻ��Ҫһ���ڴ����洢��ǰ���Ƕ��٣��±�Ϳ��Ա�ʶ
	int position;
	int count;
};

//��˳��������ú������ṹ������
struct VERTEX{
	int node;
	int flag;
};

int maxdegree;
int * edgeInf;//��Ӧ��״̬�����飬�洢���Ƕ�Ӧ���Ƿ񱻷��ʡ����ࡢ�����࣬-1��1��0
DEGREE * degreeInf;
long long bflCount;
VERTEX * orderSort;
int * outdegree;
int * indegree;
int zeroDegreeVertexCount;

void init(int md);//i�������ݵ�ǰ���������ݼ��ǵڼ������ݼ����Ա�Դ洢�ȵ�������г�ʼ��
void sort3();
void order3();
vector<pair<int, int>> redunEdgeCount();

////////////////////////////////////
//Ϊ�˵õ�TRʣ��ͼ�ӵ�ɾ������ߵĺ���
//void delRedunEdges();
////////////////////////////////////

void destroyIndex();

void init(int md){
	edgeInf = new int[edgeNum];
	maxdegree = md + 1;
	degreeInf = new DEGREE[maxdegree];//0~maxDegree
	orderSort = new VERTEX[2 * vertexNum];//�������һ��Ϊ��
	outdegree = new int[vertexNum];
	indegree = new int[vertexNum];
	for (int i = 0; i < edgeNum; i++){
		edgeInf[i] = -1;
	}
	for (int i = 0; i < vertexNum; i++){
		outdegree[i] = outEdge[i].degree;
		indegree[i] = inEdge[i].degree;
	}
	bflCount = 0;
}

//�洢����Ϣ�������0��ʼ����maxDegree
//degreeInf.degree = i
//degreeInf.positionƫ����
//degreeInf.count��Ŀ
//�ܹ���Ҫ����2n���������
//��һ�α������õ�����Ϣ�����Ƕ�Ϊi�Ľ���������е���ʼλ�ã���start��degree�ṹ��ͬ
void sort3(){
	//��һ�α���
	int sum = 0;
	int i;
	for (i = 0; i < maxdegree; i++){
		degreeInf[i].position = 0;
		degreeInf[i].count = 0;
	}
	//�ȼ�¼��Ŀ
	for (i = 0; i < vertexNum; i++){
		degreeInf[outEdge[i].degree].count++;//����߶ȶ�Ӧ������count����
		degreeInf[inEdge[i].degree].count++;
	}
	for (i = 0; i < maxdegree; i++){
		degreeInf[i].position = sum;
		sum += degreeInf[i].count;
	}
	zeroDegreeVertexCount = degreeInf[0].count;
	//������Ϊ�˽��������
	//0��ʶ���ȣ�1��ʶ���
	//�ö�Ϊ1�Ľ�㲻��������
	//�ٽ���㰴��˳���������
	//���Ѷ�Ϊ0�Ľ����룬�൱��orderSort��ǰ��һ�����ǿյ�
	for (i = 0; i < vertexNum; i++){
		if(outEdge[i].degree != 0){
			orderSort[degreeInf[outEdge[i].degree].position].node = i;
			orderSort[degreeInf[outEdge[i].degree].position++].flag = 0;
		}
		
		if(inEdge[i].degree != 0){
			orderSort[degreeInf[inEdge[i].degree].position].node = i;
			orderSort[degreeInf[inEdge[i].degree].position++].flag = 1;
		}
		
	}

	sum = 0;
	for (i = 0; i < maxdegree; i++){
		degreeInf[i].position = sum;
		sum += degreeInf[i].count;
	}
}

void order3(){
	int pCur, curV;
	int id;
	int pre, pre_id;
	int flag;
	int start, end;
	bool reachResult;

	int maxBFLCountForOneEdge = 0;
	int count = 0;

	int maxOutDegree = 0;
	int maxInDegree = 0;
	int maxDegree_ = 0;

	for (int i = zeroDegreeVertexCount; i < 2 * vertexNum; i++){
	//for(int i = 0; i < 2 * vertexNum; i++){
		pCur = orderSort[i].node;
		flag = orderSort[i].flag;
		if (flag == 0){
			if (outdegree[pCur] == 0){
				continue;
			}
			else{
				//���ﴦ���Ļ����ȿ϶��ǲ�Ϊ0��
				//�������еĳ��߽����ж�
				maxOutDegree = outEdge[pCur].degree;

				start = outEdge[pCur].start;
				end = start + outEdge[pCur].degree;

				curV = tredge[start].node;
				id = tredge[start++].id;
				if (edgeInf[id] == -1){
					edgeInf[id] = 0;
					indegree[curV]--;//ÿ����һ���±�ʱ���Ȳŵݼ�
				}

				while (start < end){
					count = 0;//�����Ǵ���һ���±ߵĿ�ʼ
					curV = tredge[start].node;
					id = tredge[start].id;
					if (edgeInf[id] != -1){//�Ѿ��жϳ����������
						start++;
						continue;
					}
					else{
						edgeInf[id] = 0;
						indegree[curV]--;
						for (int j = outEdge[pCur].start; j < start;){
							pre = tredge[j].node;
							pre_id = tredge[j].id;
							if (edgeInf[pre_id] == 1){
								j++;
								continue;
							}
							else{
								vis_cur++;
								reachResult = reach(nodes[pre], nodes[curV]);
								bflCount++;
								count++;
								if (reachResult == true){
									edgeInf[id] = 1;
									break;
								}
								else{
									j++;
								}
							}
						}
						start++;
					}
				}
				if(count > maxBFLCountForOneEdge){
					maxBFLCountForOneEdge = count;
				}
			}
		}
		else if (flag == 1){
			if (indegree[pCur] == 0){
				continue;
			}
			else{
				maxInDegree = inEdge[pCur].degree;
				start = inEdge[pCur].start;
				end = start + inEdge[pCur].degree - 1;
				curV = edgeR[end].node;
				id = edgeR[end--].id;
				if (edgeInf[id] == -1){
					edgeInf[id] = 0;
					outdegree[curV]--;
				}
				while (start <= end){
					count = 0;
					curV = edgeR[end].node;
					id = edgeR[end].id;
					if (edgeInf[id] != -1){
						end--;
						continue;
					}
					else{
						edgeInf[id] = 0;
						outdegree[curV]--;
						for (int j = inEdge[pCur].start + inEdge[pCur].degree - 1; j > end;){
							pre = edgeR[j].node;
							pre_id = edgeR[j].id;
							if (edgeInf[pre_id] == 1){
								j--;
								continue;
							}
							else{
								vis_cur++;
								reachResult = reach(nodes[curV], nodes[pre]);
								bflCount++;
								count++;
								if (reachResult == true){
									edgeInf[id] = 1;
									break;
								}
								else{
									j--;
								}
							}
						}
						end--;
					}
				}
				if(count > maxBFLCountForOneEdge){
					maxBFLCountForOneEdge = count;
				}
			}
		}
	}
	maxDegree_ = (maxOutDegree > maxInDegree) ? maxOutDegree : maxInDegree;
}

vector<pair<int, int>> redunEdgeCount(){
	int totalEdgeCounts = 0;
	int redunEdgeCounts = 0;
	for (int i = 0; i < edgeNum; i++){
		if (edgeInf[i] == 1){
			redunEdgeCounts++;
		}
		totalEdgeCounts++;
	}
	int my_pos = 0;
	vector<pair<int, int>> candidate_edges;
	for(int i = 0; i < vertexNum; i++){
		for(int j = 0; j < outEdge[i].degree; j++){
			if(edgeInf[tredge[my_pos].id] == 1){
				candidate_edges.push_back(make_pair(i, tredge[my_pos].node));
				// printf("s = %d t = %d is redundent\n", i, tredge[my_pos].node);
			}
			my_pos++;
		}
	}	
	//cout << "totalEdgeCounts = " << totalEdgeCounts << endl;
	// cout << "redunEdgeCounts = " << redunEdgeCounts << endl;
	return candidate_edges;
}

void destroyIndex(){
	delete edgeInf;
	delete degreeInf;//0~maxDegree
	delete orderSort;
	delete outdegree;
	delete indegree;
}

#endif
