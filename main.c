#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


typedef struct _Edge {
  int source;
  int target;
  int weight;
}Edge;


int *vertices_setup(int *vertices, int count) {
  int i;
  if (!vertices) vertices = (int*) malloc(count * sizeof(int));
  for (i=0; i<count; i++)
    vertices[i] = i;
  return vertices;
}

int *trees_setup(int *trees, int count) {
  int i;
  if (!trees) trees = (int*) malloc(count * sizeof(int));
  for (i=0; i<count; i++)
    trees[i] = -1;
  return trees;
}

int *components_setup(int *components, int count) {
  int i;
  if (!components) components = (int*) malloc(count * sizeof(int));
  for (i=0; i<count; i++)
    components[i] = -1;
  return components;
}


void vertices_rename(int *vertices, int count, int from, int to) {
  int i;
  for (i=0; i<count; i++)
    if (vertices[i] == from) vertices[i] = to;
}

void trees_rename(int *trees, int count, int from, int to) {
  int i;
  for (i=0; i<count; i++)
    if (trees[i] == from) trees[i] = to;
}


int components_min(int a, int b, Edge *edges) {
  if (a<0) return b;
  if (b<0) return a;
  return edges[a].weight < edges[b].weight? a : b;
}

void components_merge(int *a, int *b, int count, Edge *edges) {
  int i;
  for (i=0; i<count; i++)
    a[i] = components_min(a[i], b[i], edges);
}

void components_find(int *components, int *vertices,
    Edge *edges, int begin, int end) {
  int i;
  for (i=begin; i<end; i++) {
    Edge edge = edges[i];
    int tree_source = vertices[edge.source];
    int tree_target = vertices[edge.target];
    if (tree_source == tree_target) continue;
    int *source = &components[tree_source];
    int *target = &components[tree_target];
    if(*source < 0 || edge.weight < edges[*source].weight) *source = i;
    if(*target < 0 || edge.weight < edges[*target].weight) *target = i;
  }
}


int trees_merge(int *trees, int tree_count,
    int *components, int components_count,
    int *vertices, Edge *edges) {
  int i, merged = 0;
  for (i=0; i<components_count; i++) {
    int component = components[i];
    if (component < 0) continue;
    Edge edge = edges[component];
    int source = vertices[edge.source];
    int target = vertices[edge.target];
    if (source == target) continue;
    int from = source>target? source : target;
    int to = from==target? source : target;
    vertices_rename(vertices, tree_count, from, to);
    trees_rename(trees, tree_count, from, to);
    trees[component] = to;
    merged++;
  }
  return merged;
}


int* boruvka_mst(int vertex_count, Edge *edges, int edge_count) {
  int *components = NULL, component_count = vertex_count, merged = 0;
  int *vertices = vertices_setup(NULL, vertex_count);
  int *trees = trees_setup(NULL, edge_count);
  do {
    components = components_setup(components, component_count);
    components_find(components, vertices, edges, 0, edge_count);
    merged = trees_merge(trees, edge_count, components, component_count, vertices, edges);
  }while(merged > 0);
  free(components);
  free(vertices);
  return trees;
}


int main(int argc, char *argv[]) {
  int vertex_count, edge_count, i;
  scanf("%d %d", &vertex_count, &edge_count);
  Edge *edges = (Edge*) malloc(edge_count * sizeof(Edge));
  for (i=0; i<edge_count; i++)
    scanf("%d %d %d", &edges[i].source, &edges[i].target, &edges[i].weight);
  int *trees = boruvka_mst(vertex_count, edges, edge_count);
  int count = 0, weight = 0;
  for (i=0; i<edge_count; i++) {
    if (trees[i] < 0) continue;
    weight += edges[trees[i]].weight;
    count++;
  }
  printf("%d\n", count);
  for (i=0; i<edge_count; i++) {
    if (trees[i] < 0) continue;
    Edge edge = edges[trees[i]];
    printf("%d %d\n", edge.source, edge.target);
  }
  printf("%d\n", weight);
  free(trees);
  return 0;
}
