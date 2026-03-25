#define JSON_DEBUG_LEVEL 0

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <vector>

#include "JSON/JSON.h"
//#include "JSON/JSONParser.h"
#include "JSON/JSONPrinter.h"

// RapidJSON
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <iostream>

// ---------------------------------------------------------------------------
using namespace std;
using namespace JSON;

struct Personne : public JSONData {
public:
  std::string_view nom = "";
  uint8_t age = 0U;
  float taille = 0.0F;
  std::string_view ville = "";
  char *ptr;
  bool flag = false;

  uint8_t buffer[4] = {0};
  char liste[3][32] = {{'\0'}};
  float listef[5] = {0};
  Personne *enfant;
  std::vector<Personne> enfants;
  float coordinates[4][2];
  vector<std::array<float, 2>> coordinates2;

  // Constructeur par défaut
  Personne() : JSONData() {}
  Personne(std::string_view nom, int age, float taille, std::string_view ville,
           char *ptr, bool flag, Personne *enfant)
      : JSONData(), nom(nom), age(age), taille(taille), ville(ville), ptr(ptr),
        flag(flag), enfant(enfant) {}

  TO_JSON_FROM_JSON(nom, age, taille, ville, ptr, flag, buffer, liste, listef, enfant,
                    enfants, coordinates);
};

struct Properties : public JSONData {
  std::string_view name = "";

  TO_JSON_FROM_JSON(name);
};

struct Geometry : public JSONData {
  std::string_view type = "";
  using coordinate = std::array<float, 2>;
  using shape = std::vector<coordinate>;
  std::vector<shape> coordinates;

  TO_JSON_FROM_JSON(type, coordinates);
};

struct Feature : public JSONData {
  std::string_view type = "";
  Properties properties;
  Geometry geometry;

  TO_JSON_FROM_JSON(type, properties, geometry);
};

struct FeatureCollection : public JSONData {
  std::string_view type = "";
  std::vector<Feature> features;

  TO_JSON_FROM_JSON(type, features);
};

// // créer un float random entre min et max
// float randomFloat(float min, float max) {
//   float r = (float)rand() / (float)RAND_MAX;
//   return min + r * (max - min);
// }

// // créer un uint8_t random entre min et max
// int randomint(int min, int max) { return min + (rand() % (max - min + 1)); }

uint64_t time() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             now.time_since_epoch())
      .count();
}

void test_callback() {
  std::printf("\nTEST CALLBACK\n");
  std::printf("------------------------------------------------------------\n");
  char *ptr = (char *)"ptr";
  Personne p("Bob", 40, 1.80f, "Paris", ptr, false, nullptr);
  std::printf("BEFORE: ");
  p.toJSON();

  const char *json = "{\"ville\":\"Lyon\",\"taille\":2.1, \"age\":12.0, "
                     "\"flag\" : true, \"ptr\":null, "
                     "\"buffer\":\"AABBCCDD\",\"liste\":[\"a\", \"b\", \"c\"]}";
  JSON::ParseResult pr =
      JSON::parse(json, [&p](JSONKey key, JSONValue value, bool &stop) {
        if (key == "ville") {
          p.ville = value;
        } else if (key == "age") {
          p.age = value;
        } else if (key == "taille") {
          p.taille = value;
        } else if (key == "flag") {
          p.flag = value;
        } else if (key == "ptr") {
          p.ptr = value;
        } else if (key == "buffer") {
          value.copyTo(p.buffer);
        } else if (key == "liste") {
          strncpy(p.liste[key.index()], value.get<std::string_view>().data(),
                  value.get<std::string_view>().length());
        }
      });

  printf("\nPARSING: %s\n", json);
  printf("AFTER: ");
  p.toJSON();
  printf("\n");
  pr.print();
}

void testArrayCallback() {
  std::printf("\nTEST ARRAY CALLBACK\n");
  std::printf("------------------------------------------------------------\n");
  size_t p_length = 3;
  Personne personnes[p_length];

  const char *json = "[{\"nom\":\"Bob\",\"age\":Infinity},{\"nom\":\"Alice\","
                     "\"age\":30},{\"nom\":\"Roger\",\"age\":64}]";
  JSON::ParseResult pr = JSON::parse(
      json, [&personnes, p_length](JSONKey key, JSONValue value, bool &stop) {
        int arrayIndex = key.getArrayIndex();
        // std::printf("key = %.*s index=%d\n", (int)key.length(), key.data(),
        // arrayIndex);
        if (arrayIndex >= p_length || arrayIndex < 0) {
          return;
        }

        switch (key) {
        case "nom"_hash: {
          // std::string_view nom = value;
          // std::printf("nom = %.*s\n", (int)nom.length(), nom.data());
          personnes[arrayIndex].nom = value;
        } break;
        case "age"_hash:
          personnes[arrayIndex].age = value;
          break;
        default:
          break;
        }

        if (arrayIndex == 1)
          stop = true;
      });

  for (int i = 0; i < p_length; i++) {
    std::printf("Personne %d: ", i);
    personnes[i].toJSON();
    std::printf("\n");
  }
  pr.print();
}

void test_parsing() {
  JSON::PRINT_BUFFER_AS_HEX = false;
  std::printf("\n\nTEST PARSING & FILL\n");
  std::printf("------------------------------------------------------------\n");
  // création d’une personne
  char *ptr = (char *)"ptr";
  Personne enfant("", 10, 1.50f, "Lyon", ptr, false, nullptr);
  Personne p("Bob", 40, 1.80f, "Paris", ptr, false, &enfant);

  std::printf("BEFORE: ");
  p.toJSON();
  std::printf("\n\n");
  // création d’un JSON
  const char *json = "{\
\"ville\":\"Lyon\",\
\"age\":45.0, \
\"taille\":1.82, \
\"flag\" :false, \
\"ptr\":null, \
\"buffer\":[170, 171, 172, 173, 174], \
\"liste\":[\"a\", \"b\", \"c\"], \
\"listef\":[1.0, 2.0, 3.0, 4.0, 5.0], \
\"enfant\":{\"nom\":\"Alice\",\"age\":8}, \"unknown\":1,\
\"enfants\":[{\"nom\":\"Alice\",\"age\":8,\"taille\":1.5}, {\"nom\":\"Bob\",\"age\":10,\"taille\":1.6}],\
\"coordinates\":[[1.0, 2.0], [3.0, 4.0], [5.0, 6.0], [7.0, 8.0]],\
\"coordinates2\":[[1.0, 2.0], [3.0, 4.0], [5.0, 6.0], [7.0, 8.0]]\
}";

  printf("PARSING:%s\n\n", json);
  JSON::ParseResult result = p.fromJSON(json);
  printf("MODIFIED: ");
  p.toJSON();
  std::printf("\n\n");
  result.print();
}

void testArrayParsing() {
  std::printf("\n\nTEST ARRAY PARSING\n");
  std::printf("------------------------------------------------------------\n");

  size_t p_length = 3;
  Personne personnes[3];

  const char *json = "[{\"nom\":\"Bob\",\"age\":40},{\"nom\":\"Alice\",\"age\":"
                     "30},{\"nom\":\"Roger\",\"age\":64}]";
  uint32_t mask = 0;
  JSON::ParseResult r = JSON::parse(mask, json, personnes);
  for (int i = 0; i < p_length; i++) {
    std::printf("Personne %d: ", i);
    personnes[i].toJSON();
    std::printf("\n");
  }

  r.print();
}

void testIndexedParsing() {
  std::string_view nom;
  int age;
  
  const char*json = "{ \"nom\":\"Bob\", \"age\":40, \"ville\":\"Paris\" }";
  uint32_t mask = 0;
  JSON::ParseResult pr = JSON::parse(mask, json, "nom[0]", nom, "age[1]", age);
  pr.print();
  printf("nom = %.*s\n", (int)nom.length(), nom.data());
  printf("age = %d\n", age);
  printf("mask = %d\n", mask);
}

void testPerformance() {
  std::printf("\n\nTEST PERFORMANCE\n");
  std::printf("------------------------------------------------------------\n");

  FILE *file = fopen("tests/data.json", "r");
  if (!file) {
    std::printf("ERROR: Could not open tests/data.json\n");
    return;
  }

  char line[4096];
  int lineCount = 0;
  int errorCount = 0;

  uint64_t start = now();

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '\0' || line[0] == '\n')
      continue;

    uint32_t deviceId = 0;
    uint32_t duration = 0;

    JSON::ParseResult pr = JSON::parse(
        line, [&deviceId, &duration](JSONKey key, JSONValue value, bool &stop) {
          if (key == "deviceId") {
            deviceId = value;
          } else if (key == "duration") {
            duration = value;
          }
        });

    if (pr.error)
      errorCount++;
    lineCount++;
  }

  uint64_t elapsed = now() - start;
  fclose(file);

  std::printf("Parsed    : %d records\n", lineCount);
  std::printf("Elapsed   : %llu µs\n", (unsigned long long)elapsed);
  std::printf("Avg       : %.2f µs/record\n",
              lineCount > 0 ? (double)elapsed / lineCount : 0.0);
  std::printf("Errors    : %d\n", errorCount);
}

void parseRapidJSON(const char *json) {
  rapidjson::Document d;
  d.Parse(json);
}

// test de parsing d’un fichier GeoJSON
void testGeoJSONParsing(const char *json, bool print_result) {
  std::printf("\n\nTEST GEOJSON PARSING\n");
  std::printf("------------------------------------------------------------\n");
  FeatureCollection fc;

  JSON::ParseResult pr = fc.fromJSON(json);

  uint64_t start = now();
  parseRapidJSON(json);
  uint64_t elapsed = now() - start;
  std::printf("RapidJSON Parsing time: %lu µs\n", elapsed);
  std::printf("MyJSONParser Parsing time: %lu µs\n", pr.elapsed);
  pr.print();

  if (print_result) {
    fc.toJSON(false);
    std::printf("\n");
    size_t shapes_length = fc.features[0].geometry.coordinates.size();
    std::printf("Parsed %zu features\n", fc.features.size());
    std::printf("Shapes length =%zu\n", shapes_length);

    for (size_t i = 0; i < shapes_length; i++) {
      std::printf("Shape %zu coordinate points length =%zu\n", i,
                  fc.features[0].geometry.coordinates[i].size());
    }
  }
}

void testGeoJSONParsingSmall() {
  const char *json =
      "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
      "\"properties\":{\"name\":\"Canada\"},\"geometry\":{\"type\":\"Polygon\","
      "\"coordinates\":[[[-140.99778,41.675105],[-140.99778,83.110903],[-52."
      "648098,83.110903],[-52.631163,41.675105],[-140.99778,41.675105]],[[-140."
      "99778,41.675105],[-140.99778,83.110903],[-52.648098,83.110903],[-52."
      "631163,41.675105],[-140.99778,41.675105]]]}}]}";
  testGeoJSONParsing(json, true);
}

void testGeoJSONParsingBig() {
  // decription: read the tests/canada.json file
  FILE *file = fopen("tests/canada.json", "r");
  if (!file) {
    std::printf("ERROR: Could not open tests/canada.json\n");
  }
  fseek(file, 0, SEEK_END);
  long fsize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *json = (char *)malloc(fsize + 1);
  [[maybe_unused]] size_t len = fread(json, 1, fsize, file);
  fclose(file);
  json[fsize] = 0;

  testGeoJSONParsing(json, false);
  free(json);
}

// void testRapidJSON() {
//     // 1. Parse a JSON string into DOM.
//     const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
//     rapidjson::Document d;
//     d.Parse(json);

//     // 2. Modify it by DOM.
//     rapidjson::Value& s = d["stars"];
//     s.SetInt(s.GetInt() + 1);

//     // 3. Stringify the DOM
//     rapidjson::StringBuffer buffer;
//     rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//     d.Accept(writer);

//     // Output {"project":"rapidjson","stars":11}
//     std::cout << buffer.GetString() << std::endl;
// }

void func() {
  static int x = 0;
  printf("x = %d\n", x);
}

int main() {
  // AFFICHAGE DE LA DATE ET DE L’HEURE COURANTE
  time_t now = time(nullptr);
  std::printf("TIME:%s COMPILER:%s", ctime(&now), __VERSION__);
  std::printf("------------------------------------------------------------\n");
  // TESTS
  test_callback();
  // test_parsing();
  // testIndexedParsing();
  // testArrayParsing();
  // testArrayCallback();
  // testPerformance();
  // testRapidJSON();
  // testGeoJSONParsingSmall();
  // testGeoJSONParsingBig();

  return 0;
}