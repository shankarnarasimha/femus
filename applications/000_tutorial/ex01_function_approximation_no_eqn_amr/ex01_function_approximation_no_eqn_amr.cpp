/** tutorial/Ex1
 * This example shows how to:
 * initialize a femus application;
 * define the multilevel-mesh object mlMsh;
 * read from the file ./input/square.neu the coarse-level mesh and associate it to mlMsh;
 * add in mlMsh uniform refined level-meshes;
 * define the multilevel-solution object mlSol associated to mlMsh;
 * add in mlSol different types of finite element solution variables;
 * initialize the solution variables;
 * define vtk and gmv writer objects associated to mlSol;
 * print vtk and gmv binary-format files in ./output directory.
 **/

#include "FemusInit.hpp"
#include "MultiLevelSolution.hpp"
#include "MultiLevelProblem.hpp"
#include "VTKWriter.hpp"
#include "GMVWriter.hpp"

using namespace femus;

double InitialValueU(const std::vector < double >& x) {
  return x[0] + x[1];
}

double InitialValueP(const std::vector < double >& x) {
  return x[0];
}

double InitialValueT(const std::vector < double >& x) {
  return x[1];
}


bool SetBoundaryCondition(const std::vector < double >& x, const char SolName[], double& value, const int facename, const double time) {
  bool dirichlet = false; //dirichlet
  if( facename==1 ) dirichlet=true;
  value = 0.;
  return dirichlet;
}


bool SetRefinementFlag(const std::vector < double >& x, const int &elemgroupnumber,const int &level) {

  bool refine=0;
  if (elemgroupnumber==6 && level<1) refine=1;
  if (elemgroupnumber==7 && level<2) refine=1;
  if (elemgroupnumber==8 && level<3) refine=1;

  return refine;

}

int main(int argc, char** args) {

  // init Petsc-MPI communicator
  FemusInit mpinit(argc, args, MPI_COMM_WORLD);
  

  // Mesh - BEGIN
  
  // define multilevel mesh
  MultiLevelMesh mlMsh;
  double scalingFactor = 1.;
  // read coarse level mesh and generate finers level meshes 

  const std::string relative_path_to_build_directory =  "../../../";
  const std::string mesh_file = relative_path_to_build_directory + Files::mesh_folder_path() + "01_gambit/02_2d/square/minus0p5-plus0p5_minus0p5-plus0p5/square_4x4_quad_Four_boundary_groups_Four_volume_groups_AMR.neu";
  //mlMsh.ReadCoarseMesh("./input/box.neu", "seventh", scalingFactor);
  //mlMsh.ReadCoarseMesh("./input/square.neu", "seventh", scalingFactor);
  
  mlMsh.ReadCoarseMesh(mesh_file.c_str(), "seventh", scalingFactor); // Let mlMsh read the coarse mesh.

  /* "seventh" is the order of accuracy that is used in the gauss integration scheme
      probably in the furure it is not going to be an argument of this function   */
  unsigned numberOfUniformLevels = 1;
  unsigned numberOfSelectiveLevels = 3;
  mlMsh.RefineMesh(numberOfUniformLevels + numberOfSelectiveLevels, numberOfUniformLevels , SetRefinementFlag);
  mlMsh.PrintInfo();
  
  // Mesh - END

  
  // Solution - BEGIN
  
  // define the multilevel solution and attach the mlMsh object to it
  MultiLevelSolution mlSol(&mlMsh);
  
  // add variables to mlSol
  //mlSol.AddSolution("U", LAGRANGE, FIRST);
  mlSol.AddSolution("U", LAGRANGE, SECOND);
//   mlSol.AddSolution("V", LAGRANGE, SERENDIPITY);
//   mlSol.AddSolution("W", LAGRANGE, SECOND);
  mlSol.AddSolution("P", DISCONTINUOUS_POLYNOMIAL, ZERO);
  mlSol.AddSolution("T", DISCONTINUOUS_POLYNOMIAL, FIRST);

  mlSol.Initialize("All");    // initialize all variables to zero

  mlSol.Initialize("U", InitialValueU);
  mlSol.Initialize("P", InitialValueP);
  mlSol.Initialize("T", InitialValueT);    // note that this initialization is the same as piecewise constant element
  
  mlSol.AttachSetBoundaryConditionFunction(SetBoundaryCondition);
  mlSol.GenerateBdc("All");

  // Solution - END

  // print solutions - BEGIN
  std::vector < std::string > variablesToBePrinted;
  variablesToBePrinted.push_back("U");
  variablesToBePrinted.push_back("P");
  variablesToBePrinted.push_back("T");

  VTKWriter vtkIO(&mlSol);
  vtkIO.Write(Files::_application_output_directory, fe_fams_for_files[ FILES_CONTINUOUS_BIQUADRATIC ], variablesToBePrinted);
  vtkIO.Write(Files::_application_output_directory, fe_fams_for_files[ FILES_CONTINUOUS_LINEAR ], variablesToBePrinted);

  GMVWriter gmvIO(&mlSol);
  variablesToBePrinted.push_back("all");
  gmvIO.SetDebugOutput(true);
  gmvIO.Write(Files::_application_output_directory, fe_fams_for_files[ FILES_CONTINUOUS_BIQUADRATIC ], variablesToBePrinted);
  // print solutions - END

  return 0;
}




