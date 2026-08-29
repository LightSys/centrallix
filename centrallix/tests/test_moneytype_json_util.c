#include "obj.h"
#include <assert.h>
#include <json-c/json.h>
#include "json/json_util.h"

long long
test(char** name)
{
    *name = "moneytype_00 - json_util";

    // Positive
    json_object *json = json_tokener_parse("{\"wholepart\": 7, \"fractionpart\": 5000}");
    MoneyType money = {0};
    assert(jutilGetMoneyObject(json, &money) == 0);
    assert(money.Value == 75000);

    // Negative
    json = json_tokener_parse("{\"wholepart\": -8, \"fractionpart\": 5000}");
    assert(jutilGetMoneyObject(json, &money) == 0);
    assert(money.Value == -75000);

    // wholepart / fractionpart in different order
    json = json_tokener_parse("{\"fractionpart\": 5000, \"wholepart\": 7}");
    assert(jutilGetMoneyObject(json, &money) == 0);
    assert(money.Value == 75000);

    // Value > INT_MAX
    json = json_tokener_parse("{\"wholepart\": 220000000, \"fractionpart\": 5000}");
    assert(jutilGetMoneyObject(json, &money) == 0);
    assert(money.Value == 2200000005000ll);

    // Json objects with other attributes are not accepted
    json = json_tokener_parse("{\"wholepart\": 7, \"fractionpart\": 5000, \"nope\": 0}");
    assert(jutilGetMoneyObject(json, &money) == -1);

    return 0;
}
