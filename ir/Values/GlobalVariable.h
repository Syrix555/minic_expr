///
/// @file GlobalVariable.h
/// @brief 全局变量描述类
///
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-29
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// </table>
///
#pragma once

#include "ConstInt.h"
#include "GlobalValue.h"
#include "IRConstant.h"
#include "PointerType.h"
#include "ArrayType.h"
#include <string>

///
/// @brief 全局变量，寻址时通过符号名或变量名来寻址
///
class GlobalVariable : public GlobalValue {

public:
    ///
    /// @brief 构建全局变量，默认对齐为4字节
    /// @param _type 类型
    /// @param _name 名字
    ///
    explicit GlobalVariable(Type * _type, std::string _name, ConstInt * _init = nullptr)
        : GlobalValue(_type, _name), initVal(_init)
    {
        // 设置对齐大小
        setAlignment(4);
    }

    ///
    /// @brief  检查是否是函数
    /// @return true 是函数
    /// @return false 不是函数
    ///
    [[nodiscard]] bool isGlobalVarible() const override
    {
        return true;
    }

    ///
    /// @brief 是否属于BSS段的变量，即未初始化过的变量，或者初值都为0的变量
    /// @return true
    /// @return false
    ///
    [[nodiscard]] bool isInBSSSection() const
    {
        return this->inBSSSection;
    }

    /// @brief 检查该全局变量是否有初值
    /// @return true
    /// @return false
    [[nodiscard]] bool hasInitVal() const
    {
        return initVal != nullptr;
	}

    /// @brief 获取初值
    /// @return ConstInt*
    [[nodiscard]] ConstInt * getInitVal() const
    {
        return initVal;
    }

    /// @brief 设置初值
    /// @param init
    void setInitVal(ConstInt * init)
    {
        if (init->getVal() != 0) {
            this->initVal = init;
        	this->inBSSSection = false;
		}
	}

    ///
    /// @brief 取得变量所在的作用域层级
    /// @return int32_t 层级
    ///
    int32_t getScopeLevel() override
    {
        return 0;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    int32_t getLoadRegId() override
    {
        return this->loadRegNo;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    void setLoadRegId(int32_t regId) override
    {
        this->loadRegNo = regId;
    }

    ///
    /// @brief Declare指令IR显示
    /// @param str
    ///
    void toDeclareString(std::string & str)
    {
        auto varType = this->type;
        std::string dimStr = "";
        if (varType->isPointerType()) {
            Instanceof(ptrType, PointerType *, varType);
            auto allocatedType = ptrType->getPointeeType();
            dimStr = allocatedType->getDimString();
            str += "declare " + allocatedType->toString() + " " + getIRName() + dimStr;
        } else {
            if (hasInitVal()) {
                str = "declare " + getType()->toString() + " " + getIRName() + " = " + std::to_string(initVal->getVal());
            } else {
                str = "declare " + getType()->toString() + " " + getIRName();
			}
		}
    }

private:
    ///
    /// @brief 变量加载到寄存器中时对应的寄存器编号
    ///
    int32_t loadRegNo = -1;

    ///
    /// @brief 默认全局变量在BSS段，没有初始化，或者即使初始化过，但都值都为0
    ///
    bool inBSSSection = true;

    ///
    /// @brief 全局变量的初始值
    ///
    ConstInt * initVal = nullptr;
};
