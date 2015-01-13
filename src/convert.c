
/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "base.h"
#include "sptensor.h"
#include "ftensor.h"
#include "graph.h"
#include "io.h"
#include "matrix.h"
#include "convert.h"
#include "sort.h"


/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef struct
{
  idx_t v;
  idx_t cnt;
} kvp_t;


/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/
 static inline void __update_adj(
  idx_t const u,
  idx_t const v,
  kvp_t * * const adj,
  idx_t * const adjmkr,
  idx_t * const adjsize,
  idx_t * nedges)
{
  /* search u's adj for v */
  for(idx_t i=0; i < adjmkr[u]; ++i) {
    if(adj[u][i].v == v) {
      adj[u][i].cnt += 1;
      return;
    }
  }

  /* not found, add vertex to adj list */
  if(adjmkr[u] == adjsize[u]) {
    /* resize if necessary */
    adjsize[u] *= 2;
    adj[u] = (kvp_t *) realloc(adj[u], adjsize[u] * sizeof(kvp_t));
  }
  adj[u][adjmkr[u]].v   = v;
  adj[u][adjmkr[u]].cnt = 1;
  adjmkr[u] += 1;
  *nedges += 1;
}


static void __convert_ijk_graph(
  sptensor_t * const tt,
  char const * const ofname)
{
  FILE * fout;
  if(ofname == NULL || strcmp(ofname, "-") == 0) {
    fout = stdout;
  } else {
    fout = fopen(ofname, "w");
  }

  idx_t nvtxs = 0;
  for(idx_t m=0; m < tt->nmodes; ++m) {
    nvtxs += tt->dims[m];
  }

  /* allocate adj list */
  kvp_t ** adj = (kvp_t **) malloc(nvtxs * sizeof(kvp_t *));
  idx_t * adjmkr  = (idx_t *) malloc(nvtxs * sizeof(idx_t));
  idx_t * adjsize = (idx_t *) malloc(nvtxs * sizeof(idx_t));
  for(idx_t v=0; v < nvtxs; ++v) {
    adj[v] = (kvp_t *) malloc(2 * sizeof(kvp_t));
    adjmkr[v] = 0;
    adjsize[v] = 2;
  }
  /* marks #edges in each adj list and tells us when to resize */

  /* count edges in graph */
  idx_t nedges = 0;
  for(idx_t n=0; n < tt->nnz; ++n) {
    idx_t uoffset = 0;
    /* update each adj list */
    for(idx_t m=0; m < tt->nmodes; ++m) {
      idx_t const u = tt->ind[m][n] + uoffset;
      idx_t voffset = 0;
      /* emit triangle */
      for(idx_t m2=0; m2 < tt->nmodes; ++m2) {
        if(m != m2) {
          idx_t const v = tt->ind[m2][n] + voffset;
          __update_adj(u, v, adj, adjmkr, adjsize, &nedges);
        }
        voffset += tt->dims[m2];
      }
      uoffset += tt->dims[m];
    }
  }

  nedges /= 2;

  /* print header */
  fprintf(fout, SS_IDX" "SS_IDX" 001\n", nvtxs, nedges);

  /* now write adj list */
  for(idx_t u=0; u < nvtxs; ++u) {
    for(idx_t v=0; v < adjmkr[u]; ++v) {
      fprintf(fout, SS_IDX" "SS_IDX" ", 1+adj[u][v].v, adj[u][v].cnt);
    }
    fprintf(fout, "\n");
  }

  /* cleanup */
  if(ofname != NULL || strcmp(ofname, "-") != 0) {
    fclose(fout);
  }

  for(idx_t v=0; v < nvtxs; ++v) {
    free(adj[v]);
  }
  free(adj);
  free(adjmkr);
  free(adjsize);
}


static void __convert_fib_hgraph(
  sptensor_t * tt,
  idx_t const mode,
  char const * const ofname)
{
  ftensor_t * ft = ften_alloc(tt, 0);

  hgraph_t * hg = hgraph_fib_alloc(ft, mode);
  hgraph_write(hg, ofname);

  hgraph_free(hg);
  ften_free(ft);
}


static void __convert_fib_mat(
  sptensor_t * tt,
  idx_t const mode,
  char const * const ofname)
{
  ftensor_t * ft = ften_alloc(tt, 0);
  spmatrix_t * mat = ften_spmat(ft, mode);

  spmat_write(mat, ofname);

  spmat_free(mat);
  ften_free(ft);
}


/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
void tt_convert(
  char const * const ifname,
  char const * const ofname,
  idx_t const mode,
  splatt_convert_type const type)
{
  sptensor_t * tt = tt_read(ifname);

  switch(type) {
  case CNV_IJK_GRAPH:
    __convert_ijk_graph(tt, ofname);
    break;
  case CNV_FIB_HGRAPH:
    __convert_fib_hgraph(tt, mode, ofname);
    break;
  case CNV_FIB_SPMAT:
    __convert_fib_mat(tt, mode, ofname);
    break;
  default:
    fprintf(stderr, "SPLATT ERROR: convert type not implemented.\n");
    exit(1);
  }

  tt_free(tt);
}

