// ----------------------------------------------------------------
// Structs
// ----------------------------------------------------------------

struct BigStruct : public JSONObject {
  bool f01 = false;
  int8_t f02 = 0;
  int16_t f03 = 0;
  int32_t f04 = 0;
  int64_t f05 = 0;
  uint8_t f06 = 0;
  uint16_t f07 = 0;
  uint32_t f08 = 0;
  float f09 = 0.0f;
  double f10 = 0.0;
  char f11[32] = { 0 };
  char f12[64] = { 0 };
  std::string_view f13 = "";
  std::string_view f14 = "";
  bool f15 = false;
  int32_t f16 = 0;
  float f17 = 0.0f;
  double f18 = 0.0;
  uint8_t f19[4] = { 0 };
  uint16_t f20[4] = { 0 };
  uint32_t f21[4] = { 0 };
  float f22[4] = { 0.0f };
  int32_t f23[4] = { 0 };
  int8_t f24 = 0;
  int16_t f25 = 0;
  uint8_t f26 = 0;
  uint16_t f27 = 0;
  bool f28 = false;
  double f29 = 0.0;
  char f30[16] = { 0 };
  std::string_view f31 = "";
  int32_t f32 = 0;
  JSON_SERIALIZE_IMPL( f01,
                       f02,
                       f03,
                       f04,
                       f05,
                       f06,
                       f07,
                       f08,
                       f09,
                       f10,
                       f11,
                       f12,
                       f13,
                       f14,
                       f15,
                       f16,
                       f17,
                       f18,
                       f19,
                       f20,
                       f21,
                       f22,
                       f23,
                       f24,
                       f25,
                       f26,
                       f27,
                       f28,
                       f29,
                       f30,
                       f31,
                       f32 );
};

struct Sensor : public JSONObject {
  int id = 0;
  float temperature = 1.0f;
  bool active = false;
  char name[64] = { 0 };
  uint8_t num[3] = { 1, 2, 3 };
  JSON_SERIALIZE_IMPL( id, active, name, temperature, num );
};

struct SensorMin : public JSONObject {
  int id = 0;
  uint8_t temp = 0;
  bool active = false;
  JSON_SERIALIZE_IMPL( id, active, temp );
};

struct Config : public JSONObject {
  int version = 0;
  float interval = 0.0f;
  JSON_SERIALIZE_IMPL( version, interval );
};

struct CharArrayTest : JSONObject {
  std::string_view name = "";
  char names[3][32] = { { '\0' } };
  uint32_t numbers[2] = { 0 };

  JSON_ENCODER_IMPL( name, names, numbers );
};

struct IntegralArray : JSONObject {
  uint8_t hex[4] = { 0 };

  JSON_SERIALIZE_IMPL( hex );
};

struct STDArrayArray : JSONObject {
  using STDArray = std::array<uint8_t, 2>;
  std::array<STDArray, 2> array;

  JSON_SERIALIZE_IMPL( array );
};

struct VectorInt : JSONObject {
  std::vector<int> numbers;
  JSON_SERIALIZE_IMPL( numbers );
};

struct VectorArrayArray : JSONObject {
  using coord = std::array<uint8_t, 2>;
  using ring = std::array<coord, 2>;

  std::vector<ring> vector;

  JSON_SERIALIZE_IMPL( vector );
};

struct Child : public JSONObject {
  std::string_view nom = "";
  std::string_view prenom = "";
  uint8_t age = 0U;

  JSON_SERIALIZE_IMPL( nom, prenom, age );
};

struct Parent : public JSONObject {
  std::string_view nom = "";
  uint8_t age = 0U;
  Child child;
  Child child2;

  JSON_SERIALIZE_IMPL( nom, age, child, child2 );
};

struct Personne : public JSONObject {
  public:
  std::string_view nom = "";
  uint8_t age = 0U;
  float taille = 0.0F;
  std::string_view ville = "";
  char* ptr;
  bool flag = false;

  uint8_t buffer[4] = { 0 };
  char liste[3][32] = { { '\0' } };
  float listef[5] = { 0 };
  Personne* enfant;
  std::vector<Personne> enfants;
  float coordinates[4][2];
  vector<std::array<float, 2>> coordinates2;

  Personne(): JSONObject() {}
  Personne( std::string_view nom,
            int age,
            float taille,
            std::string_view ville,
            char* ptr,
            bool flag,
            Personne* enfant ):
      JSONObject(),
      nom( nom ),
      age( age ),
      taille( taille ),
      ville( ville ),
      ptr( ptr ),
      flag( flag ),
      enfant( enfant ) {}

  JSON_SERIALIZE_IMPL( nom,
                       age,
                       taille,
                       ville,
                       ptr,
                       flag,
                       buffer,
                       liste,
                       listef,
                       enfant,
                       enfants,
                       coordinates );
};

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