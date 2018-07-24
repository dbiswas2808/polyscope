#pragma once

#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

// Note extra quantity includes at bottom

namespace polyscope {

// Forward declare point cloud
class PointCloud;

class PointCloudQuantity {
public:
  // Base constructor which sets the name
  PointCloudQuantity(std::string name, PointCloud* pointCloud);
  virtual ~PointCloudQuantity() = 0;

  // Draw the quantity on the surface Note: for many quantities (like scalars)
  // this does nothing, because drawing happens in the mesh draw(). However
  // others (ie vectors) need to be drawn.
  virtual void draw();

  // Draw the ImGUI ui elements
  virtual void drawUI() = 0;

  // Build GUI info about a point
  virtual void buildInfoGUI(size_t pointInd);

  // === Member variables ===
  const std::string name;
  PointCloud* const parent;

  bool enabled = false; // should be set by enable() and disable()
};

// Specific subclass indicating that a quantity can create a program to draw on the points themselves
class PointCloudQuantityThatDrawsPoints : public PointCloudQuantity {
public:
  PointCloudQuantityThatDrawsPoints(std::string name, PointCloud* pointCloud);
  // Create a program to be used for drawing the points
  // CALLER is responsible for deallocating
  virtual gl::GLProgram* createProgram() = 0;

  // Do any per-frame work on the program handed out by createProgram
  virtual void setProgramValues(gl::GLProgram* program);
};


class PointCloud : public Structure {
public:
  // === Member functions ===

  // Construct a new point cloud structure
  template <class T>
  PointCloud(std::string name, const T& points);
  ~PointCloud();

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;
  virtual void preparePick() override;

  // Build the imgui display
  virtual void drawUI() override;
  virtual void drawPickUI(size_t localPickID) override;
  virtual void drawSharedStructureUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;


  // === Quantities

  // general form
  void addQuantity(PointCloudQuantity* quantity);
  void addQuantity(PointCloudQuantityThatDrawsPoints* quantity);
  PointCloudQuantity* getQuantity(std::string name, bool errorIfAbsent = true);

  // Scalars
  template <class T>
  void addScalarQuantity(std::string name, const T& values, DataType type = DataType::STANDARD);

  // Colors
  template <class T>
  void addColorQuantity(std::string name, const T& values);

  // Subsets
  // void addSubsetQuantity(std::string name, const std::vector<char>& subsetIndicators);
  // void addSubsetQuantity(std::string name, const std::vector<size_t>& subsetIndices);

  // Vectors
  template <class T>
  void addVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD);


  // Removal, etc
  // TODO use shared pointers like SurfaceMesh
  void removeQuantity(std::string name);
  void setActiveQuantity(PointCloudQuantityThatDrawsPoints* q);
  void clearActiveQuantity();
  void removeAllQuantities();

  // The points that make up this point cloud
  std::vector<glm::vec3> points;
  size_t nPoints() const { return points.size(); }

  // Misc data
  bool enabled = true;
  SubColorManager colorManager;
  static const std::string structureTypeName;

  // Small utilities
  void deleteProgram();
  void writePointsToFile(std::string filename = "");

private:
  // Quantities
  std::map<std::string, PointCloudQuantity*> quantities;

  // Visualization parameters
  Color3f initialBaseColor;
  Color3f pointColor;
  float pointRadius = 0.005;

  // Drawing related things
  gl::GLProgram* program = nullptr;
  gl::GLProgram* pickProgram = nullptr;

  PointCloudQuantityThatDrawsPoints* activePointQuantity = nullptr; // a quantity that is respondible for drawing on the
                                                                    // points themselves and overwrites `program` with
                                                                    // its own shaders

  // Helpers
  void setPointCloudUniforms(gl::GLProgram* p, bool withLight);
};


// Implementation of templated constructor
template <class T>
PointCloud::PointCloud(std::string name, const T& points_)
    : Structure(name, structureTypeName), points(standardizeVectorArray<glm::vec3, T, 3>(points_)) {

  initialBaseColor = getNextStructureColor();
  pointColor = initialBaseColor;
  colorManager = SubColorManager(initialBaseColor);

  prepare();
  preparePick();
}


// Shorthand to add a point cloud to polyscope
template <class T>
void registerPointCloud(std::string name, const T& points, bool replaceIfPresent = true) {
  PointCloud* s = new PointCloud(name, points);
  bool success = registerStructure(s);
  if (!success) delete s;
}


// Shorthand to get a point cloud from polyscope
inline PointCloud* getPointCloud(std::string name = "") {
  return dynamic_cast<PointCloud*>(getStructure(PointCloud::structureTypeName, name));
}


} // namespace polyscope


// Quantity includes
#include "polyscope/point_cloud_color_quantity.h"
#include "polyscope/point_cloud_scalar_quantity.h"
#include "polyscope/point_cloud_vector_quantity.h"

