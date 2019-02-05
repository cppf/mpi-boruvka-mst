#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


typedef struct _Edge {
  int source;
  int target;
  int weight;
}Edge;


int world_size, world_rank, root;

#define mpi_printf(fmt, ...) printf("%d: "##fmt, world_rank, __VA_ARGS__); fflush(stdout);


int *vertices_setup(int *vertices, int count) {
  int i;
  if (!vertices) vertices = (int*)malloc(count * sizeof(int));
  for (i = 0; i < count; i++)
    vertices[i] = i;
  return vertices;
}

int *trees_setup(int *trees, int count) {
  int i;
  if (!trees) trees = (int*)malloc(count * sizeof(int));
  for (i = 0; i < count; i++)
    trees[i] = -1;
  return trees;
}

int *components_setup(int *components, int count) {
  int i;
  if (!components) components = (int*)malloc(count * sizeof(int));
  for (i = 0; i < count; i++)
    components[i] = -1;
  return components;
}


void vertices_rename(int *vertices, int count, int from, int to) {
  int i;
  for (i = 0; i < count; i++)
    if (vertices[i] == from) vertices[i] = to;
}

void trees_rename(int *trees, int count, int from, int to) {
  int i;
  for (i = 0; i < count; i++)
    if (trees[i] == from) trees[i] = to;
}


int components_min(int a, int b, Edge *edges) {
  if (a < 0) return b;
  if (b < 0) return a;
  return edges[a].weight < edges[b].weight ? a : b;
}

void components_merge(int *a, int *b, int count, Edge *edges) {
  int i;
  for (i = 0; i < count; i++)
    a[i] = components_min(a[i], b[i], edges);
}

void components_find(int *components, int *vertices,
  Edge *edges, int begin, int end) {
  int i;
  for (i = begin; i < end; i++) {
    Edge edge = edges[i];
    int tree_source = vertices[edge.source];
    int tree_target = vertices[edge.target];
    if (tree_source == tree_target) continue;
    int *source = &components[tree_source];
    int *target = &components[tree_target];
    if (*source < 0 || edge.weight < edges[*source].weight) *source = i;
    if (*target < 0 || edge.weight < edges[*target].weight) *target = i;
  }
}


int trees_merge(int *trees, int tree_count,
  int *components, int components_count,
  int *vertices, Edge *edges) {
  int i, merged = 0;
  for (i = 0; i < components_count; i++) {
    int component = components[i];
    if (component < 0) continue;
    Edge edge = edges[component];
    int source = vertices[edge.source];
    int target = vertices[edge.target];
    if (source == target) continue;
    int from = source > target ? source : target;
    int to = from == target ? source : target;
    vertices_rename(vertices, tree_count, from, to);
    trees_rename(trees, tree_count, from, to);
    trees[component] = to;
    merged++;
  }
  return merged;
}


void components_mergeroot(int *a, int *b, int count, Edge *edges) {
  int i;
  for (i=1; i<world_size; i++) {
    MPI_Recv(b, count, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    components_merge(a, b, count, edges);
  }
}

void component_mergeleaf(int *a, int count, Edge *edges) {
  MPI_Send(a, count, MPI_INT, 0, 0, MPI_COMM_WORLD);
}


int* boruvka_mst(int vertex_count, Edge *edges, int edge_count) {
  int *components = NULL, component_count = vertex_count, merged = 0;
  int *vertices = vertices_setup(NULL, vertex_count);
  int *trees = trees_setup(NULL, edge_count);
  int *components_recv = (int*)malloc(component_count * sizeof(int));
  int edge_length = edge_count / world_size;
  int edge_begin = world_rank * edge_length;
  int edge_end = edge_begin + edge_length;
  do {
    components = components_setup(components, component_count);
    components_find(components, vertices, edges, edge_begin, edge_end);
    if (root) components_mergeroot(components, components_recv, component_count, edges);
    else component_mergeleaf(components, component_count, edges);
    MPI_Bcast(components, component_count, MPI_INT, 0, MPI_COMM_WORLD);
    merged = trees_merge(trees, edge_count, components, component_count, vertices, edges);
    component_count -= merged;
  } while (merged > 0);
  free(components_recv);
  free(components);
  free(vertices);
  return trees;
}


int main(int argc, char *argv[]) {
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  root = world_rank == 0;

  int vertex_count, edge_count, i;
  if (root) scanf("%d %d", &vertex_count, &edge_count);
  MPI_Bcast(&vertex_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&edge_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
  Edge *edges = (Edge*)malloc(edge_count * sizeof(Edge));
  if (root) for (i = 0; i < edge_count; i++) {
    scanf("%d %d %d", &edges[i].source, &edges[i].target, &edges[i].weight);
    edges[i].source--; edges[i].target--;
  }
  MPI_Bcast((int*)edges, edge_count*3, MPI_INT, 0, MPI_COMM_WORLD);
  int *trees = boruvka_mst(vertex_count, edges, edge_count);
  int count = 0, weight = 0;
  for (i = 0; i < edge_count; i++) {
    if (trees[i] < 0) continue;
    weight += edges[i].weight;
    count++;
  }
  if (root) {
    printf("%d\n", count);
    for (i = 0; i < edge_count; i++) {
      if (trees[i] < 0) continue;
      Edge edge = edges[i];
      printf("%d %d\n", edge.source + 1, edge.target + 1);
    }
    printf("%d\n", weight);
  }
  free(trees);
  MPI_Finalize();
  return 0;
}
