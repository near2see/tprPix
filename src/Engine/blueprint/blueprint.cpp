/*
 * ======================= blueprint.cpp =======================
 *                          -- tpr --
 *                                        CREATE -- 2019.12.04
 *                                        MODIFY -- 
 * ----------------------------------------------------------
 */
#include "blueprint.h"


//-------------------- CPP --------------------//


//-------------------- Engine --------------------//
#include "PlotBlueprint.h"
#include "YardBlueprint.h"
#include "VillageBlueprint.h"
#include "random.h"
#include "tprCast.h"
#include "blueprint_inn.h"
#include "WindClock.h"
#include "GoSpecFromJson.h"


#include "esrc_state.h"
#include "esrc_animFrameSet.h"



//-------------------- Script --------------------//


#include "tprDebug.h"



namespace blueprint {//------------------ namespace: blueprint start ---------------------//
namespace blueP_inn {//----------- namespace: blueP_inn ----------------//

    void create_new_goDataForCreate(std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &goDatasForCreate_,
                                    IntVec2 mpos_,
                                    const glm::dvec2  &dpos_,
                                    const GoSpec &goSpecRef_,
                                    const MapDataEnt &mapDataEntRef_ )noexcept;
    
}//-------------- namespace: blueP_inn end ----------------//


void init_blueprint()noexcept{
    PlotBlueprint::init_for_static();
    YardBlueprintSet::init_for_static(); // Not YardBlueprint
    VillageBlueprint::init_for_static();
    //---
    parse_plotJsonFiles();
    parse_yardJsonFiles();
    parse_villageJsonFiles();
    //---
    esrc::insertState("blueprint");
}



villageBlueprintId_t str_2_villageBlueprintId( const std::string &name_ )noexcept{
    return VillageBlueprint::str_2_villageBlueprintId( name_ );
}



// 被 job 线程调用 
void build_ecoObj_goDatasForCreate( villageBlueprintId_t villageId_,
                                    IntVec2  ecoObjMPos_,
                                    size_t   ecoObjUWeight_,
                                    std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &majorGoDatasForCreate_,
                                    std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &floorGoDatasForCreate_,
                                    std::unordered_set<fieldKey_t> &artifactFieldKeys ){

    // 使用的随机数，直接取自 相关 mpos 的 mapEntKey 值
    size_t  uWeight   {};
    IntVec2 yardMPos {};
    IntVec2 plotMPos {};
    IntVec2 entMPos  {};

    //=================//
    //    village
    //=================//
    VillageBlueprint &villageRef = VillageBlueprint::get_villageBlueprintRef( villageId_ );
    std::unique_ptr<blueP_inn::VarType_Village_Manager> varType_village_managerUPtr 
                = std::make_unique<blueP_inn::VarType_Village_Manager>( villageRef );


    uWeight = ecoObjMPos_.to_simple_uWeight();
    const MapData &villageMapData = villageRef.apply_a_random_mapData( uWeight );
    for( const auto &yEntUPtr : villageMapData.data ){ // each mapDataEnt uptr / yard


        // isRoad
        // 此功能尚未完成 ...
        // 其实不用为 road 设置特殊的功能，road 一般只有 floorGos 数据 ...

        // 并未处理 yEntUPtr 的 brokenLvl / direction 数据
        
        //=================//
        //      yard
        //=================//
        int half_ents_per_section = ENTS_PER_SECTION / 2;
        yardMPos = ecoObjMPos_ - IntVec2{half_ents_per_section, half_ents_per_section} + yEntUPtr->mposOff;
                //-- 蓝图 以 ecoobj/section left-bottom 点为 中心点

        uWeight = yardMPos.to_simple_uWeight();
        auto yardId = varType_village_managerUPtr->apply_a_yardBlueprintId( yEntUPtr->varTypeIdx, uWeight );

        YardBlueprint &yardRef = YardBlueprintSet::get_yardBlueprintRef( yardId );
        
        std::unique_ptr<blueP_inn::VarType_Yard_Manager> varType_yard_managerUPtr 
                    = std::make_unique<blueP_inn::VarType_Yard_Manager>( yardRef );

        //--- 登记 yard 占据的所有 fieldKey ---
        calc_yard_fieldKeys( artifactFieldKeys, yardMPos, yardRef.get_yardSize() );

        //---------//
        // majorGos
        //---------//
        const MapData &yardMapData_majorGos = yardRef.apply_a_random_majorGo_mapData( uWeight );
        for( const auto &pEntUPtr : yardMapData_majorGos.data ){ // each mapDataEnt uptr / plot

            const auto *varTypeDatas_Yard_majorGoPtr = yardRef.get_majorGo_varTypeDataPtr_Yard( pEntUPtr->varTypeIdx );

            plotMPos = yardMPos + pEntUPtr->mposOff;
            uWeight = plotMPos.to_simple_uWeight();

            if( !varTypeDatas_Yard_majorGoPtr->get_isPlotBlueprint() ){
                //-- 直接可以获得 mp 位置的 goSpec 数据

                const GoSpec &goSpecRef = varType_yard_managerUPtr->apply_a_majorGoSpec( pEntUPtr->varTypeIdx, uWeight );

                //--- 正式在 ecoObj 中创建 major GoDataForCreate 实例 --
                blueP_inn::create_new_goDataForCreate(majorGoDatasForCreate_,
                                                plotMPos,
                                                mpos_2_midDPos(plotMPos),
                                                goSpecRef,
                                                *(pEntUPtr) );

            }else{
                //-- mp 位置存储的是 plot 数据 --

                //=================//
                //      plot
                //=================//s
                auto plotId = varType_yard_managerUPtr->apply_a_plotBlueprintId( pEntUPtr->varTypeIdx, uWeight );
                PlotBlueprint &plotRef = PlotBlueprint::get_plotBlueprintRef( plotId );

                std::unique_ptr<blueP_inn::VarType_Plot_Manager> varType_Plot_ManagerUPtr 
                            = std::make_unique<blueP_inn::VarType_Plot_Manager>( plotRef );

                const MapData &plotMapData = plotRef.apply_a_random_mapData( uWeight );
                for( const auto &mpEntUPtr : plotMapData.data ){ // each mapDataEnt uptr / mapent

                    entMPos = plotMPos + mpEntUPtr->mposOff;
                    uWeight = entMPos.to_simple_uWeight();

                    const GoSpec &goSpecRef = varType_Plot_ManagerUPtr->apply_a_goSpec( mpEntUPtr->varTypeIdx, uWeight );

                    //--- 正式在 ecoObj 中创建 major GoDataForCreate 实例 --
                    blueP_inn::create_new_goDataForCreate(majorGoDatasForCreate_,
                                                    entMPos,
                                                    mpos_2_midDPos(entMPos),
                                                    goSpecRef,
                                                    *(mpEntUPtr) );
                }
            }
        }

        //---------//
        // floorGos
        //---------//
        // floor go 不会再细分出 plot 变量
        glm::dvec2 floorGoDPos {};

        const MapData &yardMapData_floorGos = yardRef.apply_a_random_floorGo_mapData( uWeight );
        for( const auto &pEntUPtr : yardMapData_floorGos.data ){ // each mapDataEnt uptr 

            const auto *varTypeDatas_Yard_floorGoPtr = yardRef.get_floorGo_varTypeDataPtr_Yard( pEntUPtr->varTypeIdx );

            entMPos = yardMPos + pEntUPtr->mposOff; // floorGo mid点所在 mpos
            uWeight = entMPos.to_simple_uWeight();

            const GoSpec &goSpecRef = varType_yard_managerUPtr->apply_a_floorGoSpec( pEntUPtr->varTypeIdx, uWeight );

  
            floorGoDPos = mpos_2_dpos(entMPos) + calc_floorGo_mid_dposOff( varTypeDatas_Yard_floorGoPtr->get_floorGoSize() );
            

            //--- 正式在 ecoObj 中创建 floor GoDataForCreate 实例 --
            blueP_inn::create_new_goDataForCreate(floorGoDatasForCreate_,
                                                entMPos,
                                                floorGoDPos,
                                                goSpecRef,
                                                *(pEntUPtr) );
            

        }

    }
}



// 暂时用于 chunk 生成阶段 tmp

// 目前暂未实现 floorgo 的分配

void build_yard_goDatasForCreate(   std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &majorGoDatasForCreate_,
                                    std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &floorGoDatasForCreate_,
                                        yardBlueprintId_t yardId_,
                                        IntVec2 yardMPos_,
                                        size_t yard_uWeight_
                                        ){

        size_t  uWeight  {};
        IntVec2 plotMPos {};
        IntVec2 entMPos  {};

        YardBlueprint &yardRef = YardBlueprintSet::get_yardBlueprintRef( yardId_ );
        
        std::unique_ptr<blueP_inn::VarType_Yard_Manager> varType_yard_managerUPtr 
                    = std::make_unique<blueP_inn::VarType_Yard_Manager>( yardRef );

        //---------//
        // majorGos
        //---------//
        const MapData &yardMapData_majorGos = yardRef.apply_a_random_majorGo_mapData( yard_uWeight_ );
        for( const auto &pEntUPtr : yardMapData_majorGos.data ){ // each mapDataEnt uptr / plot

            const auto *varTypeDatas_Yard_majorGoPtr = yardRef.get_majorGo_varTypeDataPtr_Yard( pEntUPtr->varTypeIdx );

            plotMPos = yardMPos_ + pEntUPtr->mposOff;
            uWeight = plotMPos.to_simple_uWeight();

            if( !varTypeDatas_Yard_majorGoPtr->get_isPlotBlueprint() ){
                //-- 直接可以获得 mp 位置的 goSpec 数据

                const GoSpec &goSpecRef = varType_yard_managerUPtr->apply_a_majorGoSpec( pEntUPtr->varTypeIdx, uWeight );

                //--- 正式在 ecoObj 中创建 major GoDataForCreate 实例 --
                blueP_inn::create_new_goDataForCreate(majorGoDatasForCreate_,
                                                plotMPos,
                                                mpos_2_midDPos(plotMPos),
                                                goSpecRef,
                                                *(pEntUPtr) );

            }else{
                //-- mp 位置存储的是 plot 数据 --

                //=================//
                //      plot
                //=================//s
                auto plotId = varType_yard_managerUPtr->apply_a_plotBlueprintId( pEntUPtr->varTypeIdx, uWeight );
                PlotBlueprint &plotRef = PlotBlueprint::get_plotBlueprintRef( plotId );

                std::unique_ptr<blueP_inn::VarType_Plot_Manager> varType_Plot_ManagerUPtr 
                            = std::make_unique<blueP_inn::VarType_Plot_Manager>( plotRef );

                const MapData &plotMapData = plotRef.apply_a_random_mapData( uWeight );
                for( const auto &mpEntUPtr : plotMapData.data ){ // each mapDataEnt uptr / mapent

                    entMPos = plotMPos + mpEntUPtr->mposOff;
                    uWeight = entMPos.to_simple_uWeight();

                    const GoSpec &goSpecRef = varType_Plot_ManagerUPtr->apply_a_goSpec( mpEntUPtr->varTypeIdx, uWeight );

                    //--- 正式在 ecoObj 中创建 major GoDataForCreate 实例 --
                    blueP_inn::create_new_goDataForCreate(majorGoDatasForCreate_,
                                                    entMPos,
                                                    mpos_2_midDPos(entMPos),
                                                    goSpecRef,
                                                    *(mpEntUPtr) );
                }
            }
        }

}







namespace blueP_inn {//----------- namespace: blueP_inn ----------------//





void create_new_goDataForCreate(std::unordered_map<mapEntKey_t, std::unique_ptr<GoDataForCreate>> &goDatasForCreate_,
                                IntVec2 mpos_,
                                const glm::dvec2  &dpos_, // 让外部计算好
                                const GoSpec &goSpecRef_,
                                const MapDataEnt &mapDataEntRef_ )noexcept{


    size_t uWeight = mpos_.to_simple_uWeight(); // 简陋的 uWeight 生成方式


    //--- 正式在 ecoObj 中创建 GoDataForCreate 实例 --
    auto outPair = goDatasForCreate_.insert({ mpos_2_key(mpos_), std::make_unique<GoDataForCreate>() });
    tprAssert( outPair.second );
    GoDataForCreate &goDRef = *(outPair.first->second);
    //--- 为 GoDataForCreate 实例 装填数据 --
    goDRef.direction = mapDataEntRef_.direction;
    goDRef.brokenLvl = mapDataEntRef_.brokenLvl;

    goDRef.goSpecId = goSpecRef_.goSpecId;
    goDRef.dpos = dpos_;

    goDRef.isMultiGoMesh = goSpecRef_.isMultiGoMesh;

    if( !goSpecRef_.isMultiGoMesh ){
        //=== single goMesh ===
        std::unique_ptr<GoDataEntForCreate> entUPtr = std::make_unique<GoDataEntForCreate>();

        //---
        entUPtr->dposOff = glm::dvec2{0.0, 0.0};
        entUPtr->windDelayIdx = calc_goMesh_windDelayIdx( goDRef.dpos + entUPtr->dposOff );
        entUPtr->subspecId = esrc::apply_a_random_animSubspecId(goSpecRef_.afsName, // e.g. "mushroom"
                                                                goSpecRef_.animLabels,
                                                                uWeight );
        
        goDRef.goMeshDataUPtrs.push_back( std::move(entUPtr) ); // move

    }else{
        //=== multi goMesh ===

        const GoSpecFromJson &goSpecFromJsonRef = GoSpecFromJson::get_goSpecFromJsonRef( goDRef.goSpecId );
        tprAssert( goSpecFromJsonRef.multiGoMeshUPtr );
        const GoMeshSet &goMeshSetRef = goSpecFromJsonRef.multiGoMeshUPtr->apply_a_goMeshSet( 
                                                                goSpecRef_.multiGoMeshType,
                                                                uWeight );

        size_t randUWeightOff = 0;
        for( const auto &jgomesh : goMeshSetRef.gomeshs ){ // each json goMesh
            randUWeightOff += 17;

            std::unique_ptr<GoDataEntForCreate> entUPtr = std::make_unique<GoDataEntForCreate>();

            //---
            entUPtr->dposOff = jgomesh.dposOff;
            entUPtr->windDelayIdx = calc_goMesh_windDelayIdx( goDRef.dpos + entUPtr->dposOff );
            entUPtr->subspecId = esrc::apply_a_random_animSubspecId(    jgomesh.animFrameSetName,
                                                                        jgomesh.animLabels,
                                                                        uWeight + randUWeightOff );

            goDRef.goMeshDataUPtrs.push_back( std::move(entUPtr) ); // move                                        
            
        }          
    }
}



    
}//-------------- namespace: blueP_inn end ----------------//
}//--------------------- namespace: blueprint end ------------------------//