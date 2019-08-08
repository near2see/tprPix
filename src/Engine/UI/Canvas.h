/*
 * ========================= Canvas.h ==========================
 *                          -- tpr --
 *                                        创建 -- 2019.03.31
 *                                        修改 -- 
 * ----------------------------------------------------------
 *  canvas 是一个 大型图元，其尺寸和位置 始终与 window 对齐。
 *  且不受 camera 影响。
 *  通过控制其唯一的 texture，来实现复杂的 动画效果
 * ----------------------------
 */
#ifndef TPR_CANVAS_H
#define TPR_CANVAS_H
/* -- 确保 glad GLFW 两个库 的引用顺序 ---
 * --    glad.h 包含了正确的OpenGL头文件（如GL/gl.h），
 * --    所以需要在其它依赖于OpenGL的头文件之前 包含 glad.h
 */
#include<glad/glad.h>  

//-------------------- CPP --------------------//
#include <vector>
#include <string>
#include <unordered_map>

//------------------- Libs --------------------//
#include "tprDataType.h"

//-------------------- Engine --------------------//
#include "tprAssert.h"
#include "IntVec.h"
#include "ShaderProgram.h"
#include "Mesh.h"



class Canvas{
public:
    Canvas() = default;

    void init(  IntVec2 *texSizePtr_,
                const std::string &lpath_vs_,
                const std::string &lpath_fs_ );

    void draw();

    inline void use_shaderProgram(){
        this->is_binded = true;
        this->shaderProgram.use_program();
    }
    inline void add_new_uniform( const std::string &name_ ){
        tprAssert( this->is_binded );
        this->shaderProgram.add_new_uniform( name_ );
    }
    inline void set_translate( float x_, float y_, float z_ ){
        tprAssert( this->is_binded );
        this->mesh.set_translate( glm::vec3{ x_, y_, z_ } );
    }
    inline GLint get_uniform_location( const std::string &name_ ){
        tprAssert( this->is_binded );
        return this->shaderProgram.get_uniform_location( name_ );
    }

private:

    //===== vals =====//
    IntVec2   *texSizePtr {}; //- = ViewingBox::screenSZ,

    Mesh           mesh {};

    ShaderProgram  shaderProgram {};
            //- Canvas 实例 往往是独一无二的，比如实现 水域动画的。
            //  这些实例，各自配有独立的 glsl 程序组。

    //===== flags =====//
    bool  is_binded {false};    //- 统一 绑定／释放

};


#endif 

