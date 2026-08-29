
struct Properties : public JSONObject {
  char name[32] = { 0 };
  JSON_SERIALIZE_IMPL( name );
};

template <size_t R, size_t C> struct GeometryLimited : public JSONObject {
  using Coordinate = std::array<float, 2>;
  using Ring = std::array<Coordinate, C>;
  char type[32] = { 0 };
  std::array<Ring, R> coordinates = {};
  JSON_SERIALIZE_IMPL( type, coordinates );
};

template <size_t R, size_t C> struct FeatureLimited : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  GeometryLimited<R, C> geometry;
  JSON_SERIALIZE_IMPL( type, properties, geometry );
};

template <size_t F, size_t R, size_t C> struct FeatureCollectionLimited
    : public JSONObject {
  std::string_view type = "";
  std::array<FeatureLimited<R, C>, F> features;
  JSON_SERIALIZE_IMPL( type, features );
};

struct FeatureSansGeometry : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  JSON_SERIALIZE_IMPL( type, properties );
};

struct FeatureCollectionSansGeometry : public JSONObject {
  std::string_view type = "";
  std::vector<FeatureSansGeometry> features;
  JSON_SERIALIZE_IMPL( type, features );
};

// 1. Déclaration principale de la structure récursive
template <size_t Dimension, typename BaseType> struct MultiVector {
  using type = std::vector<typename MultiVector<Dimension - 1, BaseType>::type>;
};

// 2. Cas d'arrêt (Spécialisation pour Dimension = 0)
template <typename BaseType> struct MultiVector<0, BaseType> {
  using type = BaseType;
};

template <size_t Dimension, typename BaseType> using MultiVector_t =
    typename MultiVector<Dimension, BaseType>::type;

template <size_t Dimension> struct Geometry : public JSONObject {
  char type[32] = { 0 };
  MultiVector_t<Dimension, std::array<float, 2>> coordinates;
  JSON_SERIALIZE_IMPL( type, coordinates );
};

template <size_t Dimension> struct Feature : JSONObject {
  char type[32];
  Properties properties;
  Geometry<Dimension> geometry;
  JSON_SERIALIZE_IMPL( type, properties, geometry );
};

template <size_t Dimension> struct FeatureCollection : JSONObject {
  std::string_view type;
  std::vector<Feature<Dimension>> features;
  JSON_SERIALIZE_IMPL( type, features );
};

using FeatureCollectionLimited10Rings = FeatureCollectionLimited<1, 10, 1>;
using MultiPoint = MultiVector_t<1, std::array<float, 2>>;
using MultiLineString = MultiVector_t<2, std::array<float, 2>>;
using Polygon = MultiVector_t<2, std::array<float, 2>>;
using MultiPolygon = MultiVector_t<3, std::array<float, 2>>;
using Coordinate = MultiVector<0, std::array<float, 2>>;
using FeatureMultipoint = Feature<1>;
using FeaturePolygon = Feature<2>;
using FeatureCollectionPolygon = FeatureCollection<2>;
using FeatureCollectionMultiPolygon = FeatureCollection<3>;