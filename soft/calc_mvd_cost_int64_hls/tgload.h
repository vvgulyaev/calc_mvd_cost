#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cinttypes>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <algorithm>
#include <memory>
#include "tgcommon.h"
//#include "c_stream.h"
//#include "xmem.h"

typedef uint8_t datatype_t;

#define DEBUG   1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define TG_QUOTE(...) #__VA_ARGS__

typedef struct {
    unsigned int var_id;
    bool is_after_capture;
    unsigned int buf_idx;
    unsigned int buf_size;
    unsigned int count_id;
} capture_marker_t;

typedef struct signal_info{
    uint32_t width;
    uint32_t content_size;
    datatype_t datatype;
    std::vector<uint8_t> content;
    bool crc_verify_mode;

    signal_info(){
        width = 0;
        content_size = 0;
        datatype = UNKNWON_TYPE;
    	crc_verify_mode = false;
    }

    void clear_data() {
        content_size = 0;
        content.clear();
    }
}signal_info_t;


class Ctgload{
public:
    Ctgload(){
        fi = NULL;
    }

    ~Ctgload(){
        if(fi != NULL){
            fclose(fi);
        }
    }

    bool open(const char *filepath)
    {
        bool rc = true;

        const char *search_path[] = {"",
                                    "../../../",
                                    "../../../capture_data/"
        };

        uint32_t signum = 0;
        std::string signame;
        signal_info_t sig_info;

        bool file_exist = false;
        std::string exist_filepath;
        for (unsigned int i=0; i<ARRAY_SIZE(search_path); i++){
            exist_filepath.assign(search_path[i]);
            exist_filepath.append(filepath);
            if (access(exist_filepath.c_str(), F_OK) == 0) {
                //std::cout << exist_filepath << " exists.\n";
                file_exist = true;
                break;
            }
        }

        if (!file_exist) {
            std::cout << filepath << " does not exist in dedicated directories .\n";
            rc = false;
            goto EXIT;
        }

        fi = fopen(exist_filepath.c_str(), "rb");
        if (fi == NULL){
            std::cout << filepath << " could not be opened.\n";
            rc = false;
            goto EXIT;
        }

        char file_signature[4];
        if (fread(file_signature, 1, sizeof(file_signature), fi) != sizeof(file_signature)) {
            fclose(fi);
            rc = false;
            goto EXIT;
        }

        if (memcmp(file_signature, "TB01", 4) != 0) {
            std::cout << "Invalid file signature\n";
            fclose(fi);
            rc = false;
            goto EXIT;
        }


        if (fread(&signum, 1, sizeof(signum), fi) != sizeof(signum)) {
            fclose(fi);
            rc = false;
            goto EXIT;
        }

        for (int i = 0; i < signum; i++) {
            uint32_t signame_len;
            if (fread(&signame_len, 1, sizeof(signame_len), fi) != sizeof(signame_len)) {
                signum = 0;
                rc = false;
                goto EXIT;
            }

            if (signame_len > 256) {
                printf("signame_len exceed 256\n");
                signum = 0;
                rc = false;
                goto EXIT;
            }
            char tmp_signame[256];
            if (fread(tmp_signame, 1, signame_len, fi) != signame_len) {
                signum = 0;
                rc = false;
                goto EXIT;
            }
            tmp_signame[signame_len] = '\0';

            signame = trim_string(tmp_signame);

            if (fread(&sig_info.width, sizeof(sig_info.width), 1, fi) != 1) {
                signum = 0;
                rc = false;
                goto EXIT;
            }

            load_order.push_back(signame);
            sinfo[signame] = sig_info;
            std::cerr << "tgload:" << signame << ",width:" << sig_info.width << '\n';
        }
        
    EXIT:
        if (!rc) {
            //exit immediately with error code
            exit(-1);
        }
        return rc;
    }

    int read_data(bool after_capture){
        while(1) {
            uint32_t var_id;
            uint32_t buf_size;

            if (fread(&var_id, sizeof(var_id), 1, fi) != 1) {
                return (feof(fi) != 0) ? EOF : 0;
            }

            if (!after_capture && var_id == TESTDATA_CAPTURE_BEFORE_END){
#if DEBUG
                std::cerr << "before capture tag found\n";
#endif
                return (feof(fi) != 0) ? EOF : 0;
            } else if (after_capture && var_id == TESTDATA_CAPTURE_AFTER_END){
#if DEBUG
                std::cerr << "after capture tag found\n";
#endif
                return (feof(fi) != 0) ? EOF : 0;
            }

            if (fread(&buf_size, sizeof(buf_size), 1, fi) != 1) {
                return ferror(fi);
            }

#if DEBUG
            if (buf_size & 0x80000000) {
                fprintf(stderr, "var_id: %d crc bufsize:%d\n", var_id, buf_size & 0x7FFFFFFF);
            } else {
                fprintf(stderr, "var_id: %d bufsize:%d\n", var_id, buf_size);
            }
#endif

            if (var_id > load_order.size()-1){
                std::cerr << "var_id out of range";
                exit(-1);
            }

            const std::string& signame = load_order[var_id];

            auto it = sinfo.find(signame);
            if (it == sinfo.end()) {
                std::cerr << "Cannot find the signame\n";
                exit(-1);
            }

            auto & test_content = it->second.content;
            auto & test_content_size = it->second.content_size;

            test_content_size = 0;

            if (buf_size & 0x80000000) {
                // handle with CRC data
                it->second.crc_verify_mode = true;
                buf_size &= 0x7FFFFFFF;

                if (buf_size > 128*1024*1024) {
                    fprintf(stderr, "unexpected bufsize during read crc data. bufsize: %x\n", buf_size);
                    exit(-1);
                }

                if (buf_size > test_content.size()) {
                    //resize the vector only if the size is smaller than read buf_size
                    test_content.reserve(buf_size);
                    test_content.resize(buf_size);
                }
                if (fread(test_content.data(), 1, 4, fi) != 4) {
                    return ferror(fi);
                }
                test_content_size = buf_size;
            } else {
            	it->second.crc_verify_mode = false;
                if (buf_size > test_content.size()) {
                    //resize the vector only if the size is smaller than read buf_size
                    test_content.reserve(buf_size);
                    test_content.resize(buf_size);
                }
                if (fread(test_content.data(), 1, buf_size, fi) != buf_size) {
                    return ferror(fi);
                }
                test_content_size = buf_size;
            }
            
            
            std::cerr << signame << " content_size:" << test_content_size << '\n';
        }
        return 0;
    }

    void pop(std::istringstream &paralist_str) {
    }

    // Variadic template function to handle multiple variables
    template <typename T, typename... Args>
    typename std::enable_if<!std::is_class<T>::value && !std::is_pointer<T>::value, void>::type
    pop(std::istringstream &paralist_str, T &data, Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "pop:T:" << (int)get_datatype(data) << ' ' << varname << ' ' << sinfo[varname].width << '\n';
#endif
        pop_data(varname, (uint8_t*)&data, sizeof(T));

        pop(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }

    template <typename T, size_t N, typename... Args>
    void pop(std::istringstream &paralist_str, T(&data)[N], Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "pop:array:" << (int)get_datatype(data) << ' ' << varname << ' ' << N << ' ' << sinfo[varname].width << '\n';
#endif
        pop_data(varname, (uint8_t*)&data[0], sizeof(T)*N);
        pop(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }

    template <typename T, typename... Args>
    typename std::enable_if<std::is_pointer<T>::value, void>::type
    pop(std::istringstream &paralist_str, T& data, Args&&... args) {
        std::string varname;
        std::getline(paralist_str, varname, ',');
        varname = trim_string(varname);
        uint32_t sigwidth = sinfo[varname].width;
        uint32_t content_size = sinfo[varname].content_size;
    #if DEBUG
            pop_data(varname, (uint8_t*)data, content_size);
        std::cerr << "pop:pointer:" << (int)get_datatype(*data) << ' ' << varname << ' ' << sinfo[varname].width << ' ' << content_size << '\n';
    #endif
        if (content_size != 0) {
            size_t num = content_size / sigwidth;
            pop_data(varname, (uint8_t*)data, content_size);
            if (num > 32 * 1024 * 1024) {
                fprintf(stderr, "content size of assign to pointer is exceed 32MB\n");
                exit(-1);
            }
        }
        pop(paralist_str, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    typename std::enable_if<std::is_class<T>::value, void>::type
    pop(std::istringstream &paralist_str, T &data, Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "pop:struct:" << (int)get_datatype(data) << ' ' << varname << ' ' << sinfo[varname].width << '\n';
#endif
        override_pop(*this, varname, data);

        pop(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }

    template <typename T, typename... Args>
    void pop(std::istringstream& paralist_str, std::vector<T>& data, Args&&... args) {
        std::string varname;
        std::getline(paralist_str, varname, ',');
        varname = trim_string(varname);
        uint32_t sigwidth = sinfo[varname].width;
        uint32_t content_size = sinfo[varname].content_size;
#if DEBUG
        std::cerr << "pop:vector:" << (int)get_datatype(*data.data()) << ' ' << varname << ' ' << sigwidth << ' ' << content_size << '\n';
#endif
        
        if (content_size != 0) {
            size_t num = content_size / sigwidth;
            data.resize(num);
            pop_data(varname, (uint8_t*)data.data(), content_size);

            if (num > 32 * 1024 * 1024) {
                fprintf(stderr, "vector size is exceed 32MB\n");
                exit(-1);
            }
        } else {
            data.clear();
        }
        
        pop(paralist_str, std::forward<Args>(args)...);
    }

    //====================================================


    void check(std::istringstream &paralist_str) {
    }

    // Variadic template function to handle multiple variables
    template <typename T, typename... Args>
    typename std::enable_if<!std::is_class<T>::value, void>::type
    check(std::istringstream &paralist_str, T &data, Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "chk:T:" << (int)get_datatype(data) << ' ' << varname << ' ' << sinfo[varname].width << '\n';
#endif
        check_data(varname, data);

        check(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }

    template <typename T, size_t N, typename... Args>
    typename std::enable_if<!std::is_class<T>::value, void>::type
    check(std::istringstream &paralist_str, T(&data)[N], Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "chk:array:" << (int)get_datatype(data) << ' ' << varname << ' ' << N << ' ' << sinfo[varname].width << '\n';
#endif
        check_data(varname, data);

        check(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }

    template <typename T, typename... Args>
    typename std::enable_if<std::is_class<T>::value, void>::type
    check(std::istringstream &paralist_str, T &data, Args&&... args){
        std::string varname;
        std::getline (paralist_str, varname, ',');
        varname = trim_string(varname);
#if DEBUG
        std::cerr << "chk:struct:" << (int)get_datatype(data) << ' ' << varname << ' ' << sizeof(data)<< '\n';
#endif
        check_data(varname, data);
        check(paralist_str, std::forward<Args>(args)...); // Recursively dump the remaining variables
    }



private:

    std::string trim_string(const std::string& str){
        auto start = str.find_first_not_of(" \t\n\r");
        auto end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos || end == std::string::npos) {
            return "";
        }
        return str.substr(start, end - start + 1);
    }

    void pop_data(const std::string &signame, uint8_t *ptr, size_t data_size)
    {
        auto &siginfo = sinfo.at(signame);
        //uint32_t width = siginfo.content.size();
        memcpy(ptr, siginfo.content.data(), data_size);
    }


    template<typename T>
    typename std::enable_if<!std::is_pointer<T>::value, void>::type
    check_data(const std::string& signame, const T& var) {
        
        auto &siginfo = sinfo.at(signame);
        uint32_t content_size = siginfo.content_size;
        if (siginfo.crc_verify_mode){
            uint32_t expect_crc_value;
            memcpy(&expect_crc_value, siginfo.content.data(), sizeof(expect_crc_value));
            uint32_t calc_crc_value = crc32((uint8_t*)&var, content_size);
            if (expect_crc_value != calc_crc_value){
                std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(T) << std::endl;
                fprintf(stderr, "actual_crc: %08X\n", calc_crc_value);
                fprintf(stderr, "expect_crc: %08X\n", expect_crc_value);
                exit(-1);
            }
        } else {
            T var2;
            pop_data(signame, (uint8_t*)&var2,  content_size);
            if (std::memcmp(&var, &var2, content_size) != 0) {
                std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(T) << std::endl;

                dump_content("actual_result", &var, sizeof(T));
                dump_content("expect_result", &var2, sizeof(T));
                
                exit(-1);
            }
        }
    }

    // New overload for pointer types
    template<typename T>
    typename std::enable_if<std::is_pointer<T>::value, void>::type
    check_data(const std::string& signame, const T& var) {
        auto &siginfo = sinfo.at(signame);
        uint32_t content_size = siginfo.content_size;
        
        if (siginfo.crc_verify_mode) {
            uint32_t expect_crc_value;
            memcpy(&expect_crc_value, siginfo.content.data(), 4);
            uint32_t calc_crc_value = crc32((uint8_t*)var, content_size);
            if (expect_crc_value != calc_crc_value) {
                std::cerr << "tgCheckError (Pointer): " << signame << " ,sizeOf: " << sizeof(T) << " ,content size:" << siginfo.content_size << std::endl;
                fprintf(stderr, "actual_crc: %08X\n", calc_crc_value);
                fprintf(stderr, "expect_crc: %08X\n", expect_crc_value);
                exit(-1);
            }
        } else {
            using BaseType = typename std::remove_pointer<T>::type;
            BaseType var2;
            pop_data(signame, (uint8_t*)&var2, content_size);
            if (std::memcmp(var, &var2, content_size) != 0) {
                std::cerr << "tgCheckError (Pointer): " << signame << " ,sizeOf: " << sizeof(BaseType) << std::endl;
                dump_content("actual_result", var, sizeof(BaseType));
                dump_content("expect_result", &var2, sizeof(BaseType));
                exit(-1);
            }
        }
    }

    FILE* fi;
    std::vector<std::string> load_order;
    std::unordered_map<std::string, signal_info_t> sinfo;

    void dump_content(const char *s, const void* buf, size_t size){
        fprintf(stderr, "%s\n", s);

        const uint8_t *pbuf8 = (const uint8_t *) buf;

        for (size_t i=0; i<size; i++){
            fprintf(stderr, "%02x ", pbuf8[i]);

            if (((i+1)%16) == 0) {
                fprintf(stderr, "\n");
            } 
        }
        fprintf(stderr, "\n");
    }
};

#define tgLoad(filepath)    Ctgload tgload;                                 \
                            if (!tgload.open(filepath)) {                   \
                                fprintf(stderr, "Open file %s failed\n", filepath);  \
                                exit(-1);                                   \
                            }

#define tgPop(...) do {\
    std::string argvstr = TG_QUOTE(__VA_ARGS__);   \
    std::istringstream sn(argvstr);             \
    int status = tgload.read_data(false);       \
    if (status == EOF) {                        \
        finish = true;                          \
        break;                                  \
    } else if (status != 0) {                   \
        fprintf(stderr, "load test data error\n");       \
        exit(-1);                               \
    }                                           \
    tgload.pop(sn, __VA_ARGS__);                \
} while (0)

#define tgCheck(...) do {\
    std::string argvstr = TG_QUOTE(__VA_ARGS__);   \
    std::istringstream sn(argvstr);             \
    int status = tgload.read_data(true);        \
    if (status == EOF) {                        \
        finish = true;                          \
    } else if (status != 0) {                   \
        fprintf(stderr, "load test data error\n");       \
        exit(-1);                               \
    }                                           \
    tgload.check(sn, __VA_ARGS__);              \
} while (0)


//specialized for struct type
template<typename T>
void override_pop(Ctgload &tgload, const std::string& var_name, T &mv){
    std::cerr << "pop func should specialized " << var_name << ',' << sizeof(T) <<'\n';
    exit(-1);
}

#ifdef TEST_MV_POC

//specialized for openhevc MvField
template<> void override_pop<MvField>(Ctgload &tgload, const std::string& var_name, MvField &mv)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_mv_0_x,";
    inner_ss << var_name << "_mv_0_y,";
    inner_ss << var_name << "_mv_1_x,";
    inner_ss << var_name << "_mv_1_y,";
    inner_ss << var_name << "_poc,";
    inner_ss << var_name << "_ref_idx,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, mv.mv[0].x, mv.mv[0].y, mv.mv[1].x, mv.mv[1].y, mv.poc, mv.ref_idx);
}

//specialized for openhevc MvField_hls
template<> void override_pop<MvField_hls>(Ctgload &tgload, const std::string& var_name, MvField_hls &mv)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_mv_0_x,";
    inner_ss << var_name << "_mv_0_y,";
    inner_ss << var_name << "_mv_1_x,";
    inner_ss << var_name << "_mv_1_y,";
    inner_ss << var_name << "_poc_0,";
    inner_ss << var_name << "_poc_1,";
    inner_ss << var_name << "_ref_idx_0,";
    inner_ss << var_name << "_ref_idx_1,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, mv.mv_0_x, mv.mv_0_y, mv.mv_1_x, mv.mv_1_y, mv.poc_0, mv.poc_1, mv.ref_idx_0, mv.ref_idx_1);
}

//specialized for openhevc Mv
template<> void override_pop<Mv>(Ctgload &tgload, const std::string& var_name, Mv &mv)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_x,";
    inner_ss << var_name << "_y,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, mv.x, mv.y);
}

//specialized for openhevc WeightValues
template<> void override_pop<WeightValues>(Ctgload &tgload, const std::string& var_name, WeightValues &wv)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_w,";
    inner_ss << var_name << "_oo,";
    inner_ss << var_name << "_offset,";
    inner_ss << var_name << "_shift,";
    inner_ss << var_name << "_round,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, wv.w, wv.oo, wv.offset, wv.shift, wv.round);
}
#endif

#ifdef CU_H_ //For Kvazaar
//specialized for kvazaar cu_info_t
template<> void override_pop<cu_info_t>(Ctgload &tgload, const std::string& var_name, cu_info_t &cu)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_type,";
    inner_ss << var_name << "_depth,";
    inner_ss << var_name << "_part_size,";
    inner_ss << var_name << "_tr_depth,";
    inner_ss << var_name << "_skipped,";
    inner_ss << var_name << "_merged,";
    inner_ss << var_name << "_merge_idx,";
    inner_ss << var_name << "_tr_skip,";
    inner_ss << var_name << "_cbf,";
    inner_ss << var_name << "_qp,";

    inner_ss << var_name << "_intra_mode,";
    inner_ss << var_name << "_intra_mode_chroma,";
    inner_ss << var_name << "_inter_mv,";
    inner_ss << var_name << "_inter_mv_ref,";
    inner_ss << var_name << "_inter_mv_cand0,";
    inner_ss << var_name << "_inter_mv_cand1,";
    inner_ss << var_name << "_inter_mv_dir,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, cu.type, cu.depth, cu.part_size, cu.tr_depth, cu.skipped, cu.merged, cu.merge_idx, cu.tr_skip, cu.cbf, cu.qp, \
        cu.intra.mode, cu.intra.mode_chroma, \
        cu.inter.mv, cu.inter.mv_ref, cu.inter.mv_cand0, cu.inter.mv_cand1, cu.inter.mv_dir);
}

//specialized for kvazaar MvField_hls
template<> void override_pop<MvField_hls>(Ctgload &tgload, const std::string& var_name, MvField_hls &mv_field)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_mv_0_x,";
    inner_ss << var_name << "_mv_0_y,";
    inner_ss << var_name << "_mv_1_x,";
    inner_ss << var_name << "_mv_1_y,";
#ifdef TEST_MV_POC
    inner_ss << var_name << "_poc,";
#endif
    inner_ss << var_name << "_ref_idx,";
#ifndef STRUCT_REMOVE_PRED_FLAG
    inner_ss << var_name << "_pred_flag,";
#endif

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, mv_field.mv_0_x, mv_field.mv_0_y, mv_field.mv_1_x, mv_field.mv_1_y,
#ifdef TEST_MV_POC
            mv_field.poc,
#endif 
            mv_field.ref_idx,
#ifndef STRUCT_REMOVE_PRED_FLAG
            mv_field.pred_flag
#endif
    );
}

//specialized for kvazaar IntraNeighbors
template<> void override_pop<IntraNeighbors>(Ctgload &tgload, const std::string& var_name, IntraNeighbors &neigh)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_numIntraNeighbor,";
    inner_ss << var_name << "_totalUnits,";
    inner_ss << var_name << "_aboveUnits,";
    inner_ss << var_name << "_leftUnits,";
    inner_ss << var_name << "_unitWidth,";
    inner_ss << var_name << "_unitHeight,";
    inner_ss << var_name << "_log2TrSize,";
    inner_ss << var_name << "_bNeighborFlags,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, neigh.numIntraNeighbor, neigh.totalUnits, neigh.aboveUnits, neigh.leftUnits, neigh.unitWidth, neigh.unitHeight, neigh.log2TrSize, neigh.bNeighborFlags);
}


//specialized for kvazaar vector2d_t
template<> void override_pop<vector2d_t>(Ctgload &tgload, const std::string& var_name, vector2d_t &vec)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_x,";
    inner_ss << var_name << "_y,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, vec.x, vec.y);
}

//specialized for kvazaar lcu_pos_hls_t
template<> void override_pop<lcu_pos_hls_t>(Ctgload &tgload, const std::string& var_name, lcu_pos_hls_t &pos)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_a0,";
    inner_ss << var_name << "_a1,";
    inner_ss << var_name << "_b0,";
    inner_ss << var_name << "_b1,";
    inner_ss << var_name << "_b2,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, pos.a0, pos.a1, pos.b0, pos.b1, pos.b2);
}

//specialized for kvazaar sao_info_t
template<> void override_pop<sao_info_t>(Ctgload &tgload, const std::string& var_name, sao_info_t &info)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_a0,";
    inner_ss << var_name << "_a1,";
    inner_ss << var_name << "_b0,";
    inner_ss << var_name << "_b1,";
    inner_ss << var_name << "_b2,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, info.type, info.applied, info.eo_class, info.ddistortion, info.merge_left_flag, info.merge_up_flag, info.band_position, info.offsets);
}

//specialized for kvazaar sharedCfg
template<> void override_pop<sharedCfg>(Ctgload &tgload, const std::string& var_name, sharedCfg &info)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_ctrl_cfg_owf,";
    inner_ss << var_name << "_ctrl_cfg_wpp,";
    inner_ss << var_name << "_ctrl_cfg_sao_type,";
    inner_ss << var_name << "_ctrl_cfg_deblock_enable,";
    inner_ss << var_name << "_ctrl_max_inter_ref_lcu_down,";
    inner_ss << var_name << "_ctrl_max_inter_ref_lcu_right,";
    inner_ss << var_name << "_ctrl_cfg_mv_constraint,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, info.ctrl_cfg_owf, info.ctrl_cfg_wpp, info.ctrl_cfg_sao_type, info.ctrl_cfg_deblock_enable, info.ctrl_max_inter_ref_lcu_down, info.ctrl_max_inter_ref_lcu_right, info.ctrl_cfg_mv_constraint);
}

#endif

#if 0
template<> void override_pop<xmem_t>(Ctgload &tgload, const std::string& var_name, xmem_t &xmem)
{

}

template<> void override_pop<child_cmd_t>(Ctgload &tgload, const std::string& var_name, child_cmd_t &cmd)
{
    std::stringstream inner_ss;
    inner_ss << var_name << "_id,";
    inner_ss << var_name << "_param,";

    std::istringstream inner_iss(inner_ss.str());

    tgload.pop(inner_iss, cmd.id, cmd.param);
}
#endif

#if 0
template<> void override_pop<c_stream_32_data>(Ctgload &cap, const std::string& var_name, c_stream_32_data &stream)
{
    std::cerr << "override_pop hls_stream!\n";
    std::istringstream inner_iss(var_name);

    std::vector<uint32_t> vec;
    cap.pop(inner_iss, vec);
    std::cerr << "dynamic arr_size:" << vec.size() << '\n';

    stream.save_data(vec);
}



template <>
void Ctgload::check_data<c_stream_32_data>(const std::string& signame, const c_stream_32_data& var) {
    std::cerr << "check data specialized: " << signame << std::endl;
    auto &siginfo = sinfo.at(signame);
    
    if (siginfo.crc_verify_mode){
        std::cerr << "Not support crc verify mode\n";
        exit(-1);
    } else {
        c_stream_32_data var2;
        std::istringstream iss(signame);
        pop(iss, var2);
        if (var2 != var) {
            std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(var) << std::endl;

            var.show_value();
            var2.show_value();

            exit(-1);
        }
    }
}

template<> void override_pop<c_stream_64_data>(Ctgload &cap, const std::string& var_name, c_stream_64_data &stream)
{
    std::cerr << "override_pop hls_stream!\n";
    std::istringstream inner_iss(var_name);

    std::vector<uint64_t> vec;
    cap.pop(inner_iss, vec);
    std::cerr << "dynamic arr_size:" << vec.size() << '\n';

    stream.save_data(vec);
}



template <>
void Ctgload::check_data<c_stream_64_data>(const std::string& signame, const c_stream_64_data& var) {
    std::cerr << "check data specialized: " << signame << std::endl;
    auto &siginfo = sinfo.at(signame);
    
    if (siginfo.crc_verify_mode){
        std::cerr << "Not support crc verify mode\n";
        exit(-1);
    } else {
        c_stream_64_data var2;
        std::istringstream iss(signame);
        pop(iss, var2);
        if (var2 != var) {
            std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(var) << std::endl;

            var.show_value();
            var2.show_value();

            exit(-1);
        }
    }
}

template<> void override_pop<c_stream_256_data>(Ctgload &cap, const std::string& var_name, c_stream_256_data &stream)
{
    std::cerr << "override_pop hls_stream!\n";
    std::istringstream inner_iss(var_name);

    std::vector<uint256_t> vec;
    cap.pop(inner_iss, vec);
    std::cerr << "dynamic arr_size:" << vec.size() << '\n';

    stream.save_data(vec);
}

template <>
void Ctgload::check_data<c_stream_256_data>(const std::string& signame, const c_stream_256_data& var) {
    std::cerr << "check data specialized: " << signame << std::endl;
    auto &siginfo = sinfo.at(signame);
    
    if (siginfo.crc_verify_mode){
        std::cerr << "Not support crc verify mode\n";
        exit(-1);
    } else {
        c_stream_256_data var2;
        std::istringstream iss(signame);
        pop(iss, var2);
        if (var2 != var) {
            std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(var) << std::endl;

            var.show_value();
            var2.show_value();

            exit(-1);
        }
    }
}

template<> void override_pop<c_stream_512_data>(Ctgload &cap, const std::string& var_name, c_stream_512_data &stream)
{
    std::cerr << "override_pop hls_stream!\n";
    std::istringstream inner_iss(var_name);

    std::vector<uint512_t> vec;
    cap.pop(inner_iss, vec);
    std::cerr << "dynamic arr_size:" << vec.size() << '\n';

    stream.save_data(vec);
}

template <>
void Ctgload::check_data<c_stream_512_data>(const std::string& signame, const c_stream_512_data& var) {
    std::cerr << "check data specialized: " << signame << std::endl;
    auto &siginfo = sinfo.at(signame);
    
    if (siginfo.crc_verify_mode){
        std::cerr << "Not support crc verify mode\n";
        exit(-1);
    } else {
        c_stream_512_data var2;
        std::istringstream iss(signame);
        pop(iss, var2);
        if (var2 != var) {
            std::cerr << "tgCheckError: " << signame << " ,sizeOf: " << sizeof(var) << std::endl;

            var.show_value();
            var2.show_value();

            exit(-1);
        }
    }
}
#endif
