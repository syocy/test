#include "delaunay.hpp"
#include <iostream>
#include <string>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
using namespace KMesh;
using namespace std;

void Delaunay::model_read(const char* model_name) {
  inFileName = model_name;
  string::size_type index = inFileName.rfind(".lua");
  if( inFileName.empty() || index == string::npos) {
    cerr << "model_read error : model_name is " << model_name << endl;
    exit(1);
  }
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaL_dofile(L, model_name);

  lua_getglobal(L, "dimension");
  dim = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_getglobal(L, "water");
  water = lua_tonumber(L, -1);
  lua_pop(L, 1);

  // Lua 変数 points_size に points のサイズを格納
  luaL_dostring(L, "points_size = #points");
  // points_size を取得
  lua_getglobal(L, "points_size");
  // points を C++ 側で格納する配列サイズを points_size にあわせる
  pts.resize(lua_tonumber(L, -1));
  lua_pop(L, 1);

  // 3次元Lua配列 points について処理
  for(unsigned i=1; i!=pts.size()+1; ++i) {
    // points[i][1] を Lua 変数 coor に格納
    char str[64];
    sprintf(str, "pt = points[%d]", i);
    luaL_dostring(L, str);
    luaL_dostring(L, "coor = pt[1]");

    // 1次元Lua配列 coor の j 番目の要素を取得
    lua_getglobal(L, "coor");
    for(unsigned j=1; j!=dim+1; ++j) {
      lua_settop(L, 1);
      lua_rawgeti(L, 1, j);
      pts[i-1][j-1] = lua_tonumber(L, 2); // 
      lua_pop(L, 1);
    }
    lua_pop(L, 1);

    luaL_dostring(L, "pt_size = #pt");
    lua_getglobal(L, "pt_size");
    unsigned pt_size = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getglobal(L, "pt");
    lua_settop(L, 1);
    lua_rawgeti(L, 1, pt_size);
    if( lua_isstring(L, 2) ) {
      pts[i-1].name = lua_tostring(L, 2);
    }
    lua_pop(L, 1);
    
    lua_pop(L, 1);
  }
  
  
  lua_close(L);
}

void Delaunay::file_out(bool isOutLua, bool isOutPs,
                        string forceFileName) const {
  string::size_type index = inFileName.rfind( "." );
  if( index == string::npos ) {
    cerr << "file_out error : inFileName is " << inFileName << endl;
    exit(1);
  }
  string headStr = inFileName.substr( 0, index ) + "_out";
  if( !forceFileName.empty() ) {
    headStr = forceFileName;
  }
  string outFileName = headStr + ".lua";
  string psFileName  = headStr + ".ps";
  if(isOutLua)
    lua_out(outFileName);
  if(isOutPs)
    ps_out(psFileName);
}

void Delaunay::lua_out(string outFileName) const {
  FILE *fp;
  fp = fopen(outFileName.c_str(), "w");
  if( fp == NULL ) {
    cerr << "fileout error : outFileName is " << outFileName << endl;
    exit(1);
  }

  fprintf(fp, "range = {");
  for(unsigned i=0; i!=dim-1; ++i) {
    fprintf(fp, "%lf, ", ptsMax[i]);
  }
  fprintf(fp, "%lf}\n\n", ptsMax[dim-1]);

  double max, ave;
  reserch_flat_ratio(max, ave);
  fprintf(fp, "max_flat_ratio     = %lf\n",   max);
  fprintf(fp, "average_flat_ratio = %lf\n\n", ave);

  index_t ntr = 1;
  index_t npt;
  fprintf(fp, "elements = {\n");
  for(index_t itr=0; itr!=trs.size(); ++itr) {
    if( !trs[itr].isValid() )
      continue;
    fprintf(fp, "   {%d, {", ntr);
    for(unsigned i=0; i!=2; ++i) {
      npt = trs[itr][i];
      npt = (npt>=iSuperPt ? npt-3 : npt);
      fprintf(fp, "%d, ", npt+1);
    }
    npt = trs[itr][2];
    npt = (npt>=iSuperPt ? npt-3: npt);
    fprintf(fp, "%d}},\n", npt+1);
    ++ntr;
  }
  fprintf(fp, "}\n\n");

  npt = 1;
  fprintf(fp, "points = {\n");
  for(index_t ipt=0; ipt!=pts.size(); ++ipt) {
    if( !pts[ipt].isValid() )
      continue;
    fprintf(fp, "   {%d, {", npt);
    for(unsigned i=0; i!=dim-1; ++i) {
      fprintf(fp, "%6lf, ", pts[ipt][i]);
    }
    fprintf(fp, "%6lf}},\n", pts[ipt][dim-1]);
    ++npt;
  }
  fprintf(fp, "}\n");

  fclose(fp);
}

void Delaunay::ps_out(string outFileName) const {
  FILE *fp = fopen(outFileName.c_str(), "w");
  if(fp==NULL) {
    cerr << "ps_out : file open error\n" << endl;
    exit(1);
  }

  unsigned psSize[] = {200, 200};
//   float lrgb[] = {0.0, 0.05, 0.3};
//   float prgb[] = {0.2, 0.7, 0.1};
  // float lcmyk[] = {0, 0.78, 0.53, 0.62};
  // float pcmyk[] = {0, 0.83, 0.83, 0.09};
  // float lcmyk[] = {0.4, 0.2, 0.0, 0.0};
  // float pcmyk[] = {0.6, 0.0, 0.6, 0.0};
  float lcmyk[] = {0.89, 0.0, 0.89, 0.1};
  float pcmyk[] = {0.89, 0.45, 0.0, 0.1};
  float lineWidth = 0.5;
  double max=0;
  for(unsigned i=0; i!=dim; ++i) {
    if(max<ptsMax[i]) {
      max = ptsMax[i];
    }
  }
  fprintf(fp, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  fprintf(fp, "%%%%BoundingBox: 0 0 %d %d\n\n", psSize[0], psSize[1]);
  fprintf(fp, "10 10 translate\n");
  fprintf(fp, ".9 .9 scale\n");
  fprintf(fp, "%f setlinewidth\n\n", lineWidth);
  fprintf(fp, "%.3f %.3f %.3f %.3f setcmykcolor\n",
          lcmyk[0], lcmyk[1], lcmyk[2], lcmyk[3]);
  for(index_t itr=0; itr!=trs.size(); ++itr) {
    if(!trs[itr].isValid())
      continue;
    index_t ipt;
    for(unsigned i=0; i!=3; ++i) {
      ipt = trs[itr][i];
      for(unsigned j=0; j!=dim; ++j) {
        fprintf(fp, "%3d ",
                static_cast<int>(round(pts[ipt][j] * psSize[j] / max)));
      }
      if(i==0)
        fprintf(fp, "moveto\n");
      else
        fprintf(fp, "lineto\n");
    }
    fprintf(fp, "closepath gsave\n");
    fprintf(fp, ".9 setgray fill\n");
    fprintf(fp, "grestore stroke\n\n");
  }
  fprintf(fp, "%.3f %.3f %.3f %.3f setcmykcolor\n",
          pcmyk[0], pcmyk[1], pcmyk[2], pcmyk[3]);
  for(index_t ipt=0; ipt!=iSuperPt; ++ipt) {
    for(unsigned j=0; j!=dim; ++j) {
      fprintf(fp, "%3d ",
              static_cast<int>(round(pts[ipt][j] * psSize[j] / max)));
    }
    fprintf(fp, "moveto\n");
    fprintf(fp, "-4 0 rlineto\n");
    fprintf(fp, "4 -4 rlineto\n");
    fprintf(fp, "4 4 rlineto\n");
    fprintf(fp, "-4 4 rlineto\n");
    fprintf(fp, "-4 -4 rlineto\n");
    fprintf(fp, "closepath fill newpath\n\n");
  }
  
  fclose(fp);
}

void KMesh::out_read(const char* out_name,
                     double point[][3], int node[][4]) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaL_dofile(L, out_name);

  luaL_dostring(L, "ets_size = #elements");
  lua_getglobal(L, "ets_size");
  int ets_size = lua_tonumber(L, -1);
  lua_pop(L, 1);

  luaL_dostring(L, "pts_size = #points");
  lua_getglobal(L, "pts_size");
  int pts_size = lua_tonumber(L, -1);
  lua_pop(L, 1);

  for(int i=1; i!=ets_size+1; ++i) {
    char str[64];
    sprintf(str, "et = elements[%d]", i);
    luaL_dostring(L, str);
    luaL_dostring(L, "itr = et[1]");
    lua_getglobal(L, "itr");
    index_t itr = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    luaL_dostring(L, "nodes = et[2]");
    lua_getglobal(L, "nodes");
    for(int j=1; j!=3+1;  ++j) {
      lua_settop(L, 1);
      lua_rawgeti(L, 1, j);
      node[itr][j] = lua_tonumber(L, 2);
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }

  for(int i=1; i!=pts_size+1; ++i) {
    char str[64];
    sprintf(str, "pt = points[%d]", i);
    luaL_dostring(L, str);
    luaL_dostring(L, "ipt = pt[1]");
    lua_getglobal(L, "ipt");
    index_t ipt = lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    luaL_dostring(L, "coor = pt[2]");
    lua_getglobal(L, "coor");
    for(int j=1; j!=2+1;  ++j) {
      lua_settop(L, 1);
      lua_rawgeti(L, 1, j);
      point[ipt][j] = lua_tonumber(L, 2);
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }

  lua_close(L);
}

// int main() {
//   Delaunay dl;
//   dl.model_read("test_model.lua");
//   return 0;
// }
