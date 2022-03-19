#include <iostream>
#include <string>

#include <vector>

#define AG_TEST_MODE
#include "AgHashTable.h"

constexpr uint64_t   largePrime     = 1e9 + 7;
constexpr uint64_t   m              = 18446744073709551557ULL;

uint64_t
bin_pow (uint64_t pBase, uint64_t pPow)
{
    uint64_t                    res = 1;

    while (pPow > 0) {

        if (pPow & 1) {
            res     *= pBase;
            res     %= m;
        }

        pPow    >>= 1;
        pBase   *= pBase;
        pBase   %= m;
    }

    return res;
}

uint64_t
string_hash_func64 (const std::string &pKey)
{
    uint64_t                    res         = 0;

    for (uint64_t i = 0, coeff = 1; i < pKey.size (); ++i, coeff = (coeff * largePrime) % m) {
        res += pKey[i] * coeff;
        res %= m;
    }

    return res;
}

uint32_t
string_hash_func32 (const std::string &pKey)
{
    return (uint32_t)(string_hash_func32 (pKey) & (0xffffffff));
}

uint16_t
string_hash_func16 (const std::string &pKey)
{
    return (uint16_t)(string_hash_func64 (pKey) & (0xffff));
}

uint16_t
id (int64_t i)
{
    return *(uint16_t *)(&i);
}

int
main (void)
{
    // find two string which have the same hash

    // AgHashTable<std::string, string_hash_func16>  table;

    // std::string                                 ar[] = {
    //     "barjam@optonline.net",
    //     "wikinerd@comcast.net",
    //     "lukka@outlook.com",
    //     "trieuvan@msn.com",
    //     "seebs@live.com",
    //     "trygstad@mac.com",
    //     "subir@live.com",
    //     "bcevc@sbcglobal.net",
    //     "hahsler@optonline.net",
    //     "chaffar@optonline.net",
    //     "wetter@comcast.net",
    //     "tbeck@sbcglobal.net",
    //     "dowdy@msn.com",
    //     "zilla@icloud.com",
    //     "osrin@msn.com",
    //     "willg@sbcglobal.net",
    //     "richard@yahoo.ca",
    //     "overbom@sbcglobal.net",
    //     "oster@msn.com",
    //     "agapow@yahoo.com",
    //     "jacks@verizon.net",
    //     "dprice@verizon.net",
    //     "koudas@comcast.net",
    //     "timtroyr@yahoo.com",
    //     "denism@hotmail.com",
    //     "madler@msn.com",
    //     "smallpaul@comcast.net",
    //     "cvrcek@att.net",
    //     "hahsler@me.com",
    //     "jmgomez@outlook.com",
    //     "tarreau@me.com",
    //     "mnemonic@optonline.net",
    //     "bruck@live.com",
    //     "sethbrown@msn.com",
    //     "gozer@verizon.net",
    //     "ianbuck@mac.com",
    //     "jaarnial@aol.com",
    //     "mcrawfor@aol.com",
    //     "mcraigw@msn.com",
    //     "jsbach@msn.com",
    //     "donev@sbcglobal.net",
    //     "report@gmail.com",
    //     "fmtbebuck@aol.com",
    //     "jfinke@hotmail.com",
    //     "keutzer@live.com",
    //     "oneiros@me.com",
    //     "vsprintf@icloud.com",
    //     "szymansk@outlook.com",
    //     "drolsky@gmail.com",
    //     "fhirsch@hotmail.com",
    //     "keiji@att.net",
    //     "sarahs@mac.com",
    //     "pedwards@sbcglobal.net",
    //     "kalpol@gmail.com",
    //     "amaranth@mac.com",
    //     "kronvold@yahoo.ca",
    //     "quinn@gmail.com",
    //     "quinn@yahoo.ca",
    //     "gomor@verizon.net",
    //     "carmena@msn.com",
    //     "caronni@icloud.com",
    //     "mcnihil@hotmail.com",
    //     "dburrows@aol.com",
    //     "joehall@att.net",
    //     "morain@optonline.net",
    //     "kildjean@mac.com",
    //     "gavinls@yahoo.com",
    //     "tmaek@verizon.net",
    //     "tromey@icloud.com",
    //     "matloff@mac.com",
    //     "meder@gmail.com",
    //     "crimsane@optonline.net",
    //     "lcheng@optonline.net",
    //     "emmanuel@outlook.com",
    //     "agolomsh@yahoo.ca",
    //     "guialbu@live.com",
    //     "nacho@gmail.com",
    //     "russotto@mac.com",
    //     "mbrown@me.com",
    //     "russotto@yahoo.ca",
    //     "tezbo@yahoo.com",
    //     "luvirini@outlook.com",
    //     "carcus@att.net",
    //     "nachbaur@icloud.com",
    //     "sinclair@me.com",
    //     "juliano@aol.com",
    //     "miturria@outlook.com",
    //     "koudas@optonline.net",
    //     "gtaylor@live.com",
    //     "pemungkah@live.com",
    //     "kwilliams@hotmail.com",
    //     "tellis@comcast.net",
    //     "chaikin@optonline.net",
    //     "dexter@att.net",
    //     "iapetus@gmail.com",
    //     "cgarcia@hotmail.com",
    //     "dprice@hotmail.com",
    //     "lbecchi@me.com",
    //     "sharon@att.net",
    //     "tsuruta@mac.com",
    //     "ccohen@aol.com",
    //     "tkrotchko@me.com",
    //     "bachmann@me.com",
    //     "solomon@gmail.com",
    //     "ubergeeb@me.com",
    //     "clkao@outlook.com",
    //     "tfinniga@comcast.net",
    //     "rgarton@verizon.net",
    //     "jugalator@gmail.com",
    //     "mschwartz@outlook.com",
    //     "lushe@msn.com",
    //     "tubajon@yahoo.ca",
    //     "goresky@verizon.net",
    //     "panolex@yahoo.com",
    //     "ilyaz@yahoo.ca",
    //     "lbaxter@msn.com",
    //     "pierce@att.net",
    //     "magusnet@optonline.net",
    //     "jacks@yahoo.com",
    //     "greear@aol.com",
    //     "steve@comcast.net",
    //     "phizntrg@outlook.com",
    //     "jaffe@hotmail.com",
    //     "jramio@optonline.net",
    //     "jeteve@gmail.com",
    //     "kimvette@aol.com",
    //     "pavel@me.com",
    //     "gbailie@sbcglobal.net",
    //     "engelen@optonline.net",
    //     "jbryan@me.com",
    //     "dkeeler@verizon.net",
    //     "amcuri@me.com",
    //     "grinder@sbcglobal.net",
    //     "sopwith@live.com",
    //     "arebenti@gmail.com",
    //     "timtroyr@live.com",
    //     "eimear@optonline.net",
    //     "epeeist@comcast.net",
    //     "pappp@optonline.net",
    //     "privcan@yahoo.ca",
    //     "ivoibs@msn.com",
    //     "chrwin@hotmail.com",
    //     "miami@yahoo.ca",
    //     "calin@yahoo.com",
    //     "dhwon@aol.com",
    //     "ducasse@live.com",
    //     "mjewell@att.net",
    //     "maikelnai@yahoo.ca",
    //     "philb@outlook.com",
    //     "mfleming@live.com",
    //     "panolex@me.com",
    //     "sscorpio@gmail.com",
    //     "janusfury@att.net",
    //     "zeller@gmail.com",
    //     "bogjobber@outlook.com",
    //     "draper@icloud.com",
    //     "sumdumass@me.com",
    //     "report@hotmail.com",
    //     "gomor@optonline.net",
    //     "durist@comcast.net",
    //     "dinther@mac.com",
    //     "dcoppit@yahoo.com",
    //     "frostman@live.com",
    //     "torgox@aol.com",
    //     "dmbkiwi@msn.com",
    //     "liedra@hotmail.com",
    //     "yxing@icloud.com",
    //     "dinther@verizon.net",
    //     "jbuchana@me.com",
    //     "alfred@icloud.com",
    //     "uqmcolyv@aol.com",
    //     "trygstad@msn.com",
    //     "crobles@comcast.net",
    //     "danny@icloud.com",
    //     "horrocks@me.com",
    //     "druschel@comcast.net",
    //     "debest@hotmail.com",
    //     "madanm@hotmail.com",
    //     "ilikered@comcast.net",
    //     "chronos@mac.com",
    //     "murty@sbcglobal.net",
    //     "madanm@mac.com",
    //     "jonadab@gmail.com",
    //     "michiel@comcast.net",
    //     "pedwards@gmail.com",
    //     "naoya@verizon.net",
    //     "jbailie@sbcglobal.net",
    //     "djpig@gmail.com",
    //     "report@msn.com",
    //     "heckerman@verizon.net",
    //     "hahsler@msn.com",
    //     "aegreene@comcast.net",
    //     "roesch@outlook.com",
    //     "rogerspl@hotmail.com",
    //     "pspoole@msn.com",
    //     "choset@icloud.com",
    //     "gordonjcs@live.com",
    //     "pierce@mac.com",
    //     "agapow@aol.com",
    //     "danny@gmail.com",
    //     "sabren@verizon.net",
    //     "njpayne@optonline.net",
    //     "niknejad@me.com",
    //     "subir@msn.com",
    //     "dpitts@hotmail.com",
    //     "aglassis@yahoo.com",
    //     "jfmulder@comcast.net",
    //     "nicktrig@sbcglobal.net",
    //     "feamster@live.com",
    //     "fwiles@sbcglobal.net",
    //     "unreal@verizon.net",
    //     "gmcgath@msn.com",
    //     "esokullu@yahoo.ca",
    //     "arachne@live.com",
    //     "augusto@icloud.com",
    //     "jelmer@aol.com",
    //     "mhassel@outlook.com",
    //     "nasor@icloud.com",
    //     "nullchar@outlook.com",
    //     "isorashi@outlook.com",
    //     "rfisher@msn.com",
    //     "jshearer@yahoo.ca",
    //     "moonlapse@icloud.com",
    //     "pizza@hotmail.com",
    //     "cumarana@gmail.com",
    //     "gordonjcp@live.com",
    //     "gmcgath@sbcglobal.net",
    //     "denism@yahoo.com",
    //     "shawnce@live.com",
    //     "yangyan@yahoo.ca",
    //     "fraterk@mac.com",
    //     "geekgrl@att.net",
    //     "studyabr@mac.com",
    //     "danneng@hotmail.com",
    //     "teverett@me.com",
    //     "jfreedma@verizon.net",
    //     "skajan@live.com",
    //     "kramulous@yahoo.ca",
    //     "citadel@optonline.net",
    //     "dleconte@outlook.com",
    //     "ajohnson@me.com",
    //     "temmink@att.net",
    //     "brickbat@aol.com",
    //     "gator@hotmail.com",
    //     "zilla@comcast.net",
    //     "zilla@me.com",
    //     "jgoerzen@outlook.com",
    //     "dburrows@icloud.com",
    //     "lushe@comcast.net",
    //     "raines@att.net",
    //     "durist@optonline.net",
    //     "esasaki@live.com",
    //     "tfinniga@icloud.com",
    //     "philb@yahoo.ca",
    //     "jwarren@mac.com",
    //     "world@comcast.net"
    // };


    // for (auto &e : ar) {
    //     if (table.insert (e) == false) {
    //         std::cout << "Could not insert " << e << std::endl;
    //         return 1;
    //     }
    // }

    // for (auto &e : ar) {
    //     if (table.find (e) == false) {
    //         std::cout << "Could not find " << e << std::endl;
    //         return 1;
    //     }
    // }

    // std::cout << "Inserted and found all\n";

    AgHashTable<int32_t, id>    table;

    for (int i = -100'000; i < 100'000; ++i) {
        if (table.insert (i) != true) {
            std::cout << "Could not insert " << i << std::endl;
        }
    }

    std::cout << "Inserted all elements\n";

    for (int i = -100'000; i < 100'000; ++i) {
        if (table.find (i) != true) {
            std::cout << "Could not find " << i << std::endl;
        }
    }

    std::cout << "Found all elements\n";

    for (int i = -100'000; i < 100'000; ++i) {
        if (table.erase (i) != true) {
            std::cout << "Could not erase " << i << std::endl;
        }
    }

    std::cout << "Erased all elements\n";

    return 0;
}
