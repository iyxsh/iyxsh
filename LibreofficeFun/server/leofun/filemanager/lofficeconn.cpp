#include "lofficeconn.h"
#include "../logger/logger.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace filemanager
{
    // 静态成员变量定义
    uno::Reference<uno::XComponentContext> LibreOfficeConnectionManager::mContext;
    uno::Reference<frame::XComponentLoader> LibreOfficeConnectionManager::mLoader;
    bool LibreOfficeConnectionManager::mIsConnected = false;

    // 实现连接初始化
    bool LibreOfficeConnectionManager::initialize()
    {
        // 如果已经连接，直接返回成功
        if (mIsConnected)
        {
            return true;
        }

        // 在每次尝试前清理可能存在的旧资源
        release();
        
        // 尝试重新连接最多3次
        int maxRetries = 3;
        for (int attempt = 1; attempt <= maxRetries; ++attempt)
        {
            if (attempt > 1)
            {
                std::cout << "Retrying LibreOffice connection (attempt " << attempt << "/" << maxRetries << ")..." << std::endl;
                // 等待一段时间再重试
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 * attempt));
            }

            // 为每次尝试定义局部变量
            uno::Reference<uno::XComponentContext> xLocalContext;
            uno::Reference<lang::XMultiComponentFactory> xLocalMCF;
            uno::Reference<bridge::XUnoUrlResolver> xUrlResolver;
            uno::Reference<uno::XComponentContext> xContext;
            uno::Reference<lang::XMultiComponentFactory> xMCF_base;
            uno::Reference<frame::XComponentLoader> loader;

            try
            {
                // 检查并输出 URE_BOOTSTRAP 环境变量
                const char *ure_bootstrap = std::getenv("URE_BOOTSTRAP");
                if (ure_bootstrap)
                    std::cout << "[UNO] URE_BOOTSTRAP=" << ure_bootstrap << std::endl;
                else
                    std::cout << "[UNO] URE_BOOTSTRAP not set!" << std::endl;

                // 创建本地组件上下文
                try {
                    xLocalContext = cppu::defaultBootstrap_InitialComponentContext();
                } catch (const std::exception& e) {
                    std::cerr << "Exception during bootstrap: " << e.what() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception during bootstrap" << std::endl;
                    continue; // 重试
                }
                
                // 检查本地组件上下文是否有效
                if (!xLocalContext.is())
                {
                    std::cerr << "Failed to create local component context" << std::endl;
                    continue; // 重试
                }

                // 获取本地服务管理器
                try {
                    xLocalMCF = xLocalContext->getServiceManager();
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception getting service manager: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception getting service manager" << std::endl;
                    continue; // 重试
                }
                
                // 检查本地服务管理器是否有效
                if (!xLocalMCF.is())
                {
                    std::cerr << "Failed to get local service manager" << std::endl;
                    continue; // 重试
                }

                // 创建UnoUrlResolver实例
                uno::Reference<uno::XInterface> xResolverIface;
                try {
                    xResolverIface = xLocalMCF->createInstanceWithContext(
                        convertStringToOUString("com.sun.star.bridge.UnoUrlResolver"), xLocalContext);
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception creating UnoUrlResolver: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception creating UnoUrlResolver" << std::endl;
                    continue; // 重试
                }
                
                // 检查UnoUrlResolver实例是否有效
                if (!xResolverIface.is())
                {
                    std::cerr << "Failed to create UnoUrlResolver instance" << std::endl;
                    continue; // 重试
                }

                // 查询XUnoUrlResolver接口
                try {
                    xUrlResolver = uno::Reference<bridge::XUnoUrlResolver>(xResolverIface, uno::UNO_QUERY);
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception querying XUnoUrlResolver: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception querying XUnoUrlResolver" << std::endl;
                    continue; // 重试
                }
                
                // 检查XUnoUrlResolver接口是否有效
                if (!xUrlResolver.is())
                {
                    std::cerr << "Failed to query XUnoUrlResolver interface" << std::endl;
                    continue; // 重试
                }

                // 连接到LibreOffice服务
                uno::Reference<uno::XInterface> xCtxIface;
                try
                {
                    xCtxIface = xUrlResolver->resolve(
                        convertStringToOUString("uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"));
                }
                catch (const uno::Exception &e)
                {
                    std::cerr << "XUnoUrlResolver(socket) failed: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    std::cerr << "Please ensure LibreOffice is running with: soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                    std::cerr << "You can also try: soffice --headless --accept=\"socket,host=0.0.0.0,port=2002;urp;\" (for remote access)" << std::endl;
                    continue; // 重试
                }
                catch (...)
                {
                    std::cerr << "Unknown exception during resolve" << std::endl;
                    continue; // 重试
                }

                // 检查解析结果是否有效
                if (!xCtxIface.is())
                {
                    std::cerr << "Failed to resolve LibreOffice context interface" << std::endl;
                    continue; // 重试
                }

                // 获取组件上下文
                try {
                    xContext = uno::Reference<uno::XComponentContext>(xCtxIface, uno::UNO_QUERY);
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception querying XComponentContext: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception querying XComponentContext" << std::endl;
                    continue; // 重试
                }
                
                // 检查组件上下文是否有效
                if (!xContext.is())
                {
                    std::cerr << "XComponentContext is null! Please ensure LibreOffice is running with correct parameters." << std::endl;
                    continue; // 重试
                }

                // 获取服务管理器
                try {
                    xMCF_base = xContext->getServiceManager();
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception getting context service manager: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception getting context service manager" << std::endl;
                    continue; // 重试
                }
                
                // 检查服务管理器是否有效
                if (!xMCF_base.is())
                {
                    std::cerr << "getServiceManager() returned null!" << std::endl;
                    continue; // 重试
                }

                // 创建桌面服务
                try {
                    // 直接通过服务管理器创建Desktop实例
                    uno::Reference<uno::XInterface> desktopInterface = xMCF_base->createInstanceWithContext(
                        convertStringToOUString("com.sun.star.frame.Desktop"), xContext);
                    
                    // 检查Desktop实例是否有效
                    if (desktopInterface.is()) {
                        loader = uno::Reference<frame::XComponentLoader>(desktopInterface, uno::UNO_QUERY);
                    } else {
                        std::cerr << "Failed to create Desktop instance - returned null interface" << std::endl;
                        continue; // 重试
                    }
                } catch (const uno::Exception& e) {
                    std::cerr << "UNO Exception while creating Desktop instance: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                    continue; // 重试
                } catch (const std::exception& e) {
                    std::cerr << "STD Exception while creating Desktop instance: " << e.what() << std::endl;
                    continue; // 重试
                } catch (...) {
                    std::cerr << "Unknown exception while creating Desktop instance" << std::endl;
                    continue; // 重试
                }
                
                // 检查loader是否有效
                if (!loader.is())
                {
                    std::cerr << "XComponentLoader is null!" << std::endl;
                    continue; // 重试
                }

                // 所有组件都成功创建，更新成员变量
                mContext = xContext;
                mLoader = loader;
                mIsConnected = true;
                std::cout << "Successfully connected to LibreOffice service" << std::endl;
                return true;
            }
            catch (const uno::Exception &e)
            {
                std::cerr << "UNO Exception during LibreOffice initialization: " << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() << std::endl;
                std::cerr << "Please ensure LibreOffice is running with: soffice --headless --accept=\"socket,host=127.0.0.1,port=2002;urp;\"" << std::endl;
                std::cerr << "You can also try: soffice --headless --accept=\"socket,host=0.0.0.0,port=2002;urp;\" (for remote access)" << std::endl;
                // 清理当前尝试中创建的资源
                release();
                continue; // 重试
            }
            catch (const std::exception &e)
            {
                std::cerr << "STD Exception during LibreOffice initialization: " << e.what() << std::endl;
                // 清理当前尝试中创建的资源
                release();
                continue; // 重试
            }
            catch (...)
            {
                std::cerr << "Unknown exception during LibreOffice initialization" << std::endl;
                // 清理当前尝试中创建的资源
                release();
                continue; // 重试
            }
        }

        // 所有重试都失败了
        std::cerr << "Failed to connect to LibreOffice after " << maxRetries << " attempts" << std::endl;
        mIsConnected = false;
        return false;
    }

    // 获取组件加载器
    uno::Reference<frame::XComponentLoader> LibreOfficeConnectionManager::getComponentLoader()
    {
        // 如果没有连接，先尝试初始化连接
        if (!mIsConnected)
        {
            if (!initialize())
            {
                return nullptr;
            }
        }

        // 验证现有连接是否仍然有效
        try
        {
            // 检查上下文和加载器是否还有效
            if (mContext.is() && mLoader.is())
            {
                // 尝试执行多个操作来验证连接的健壮性
                uno::Reference<lang::XMultiComponentFactory> xMCF_base = mContext->getServiceManager();
                if (xMCF_base.is())
                {
                    // 进一步检查服务工厂是否能创建简单对象
                    try
                    {
                        uno::Reference<uno::XInterface> testInstance = xMCF_base->createInstanceWithContext(
                            convertStringToOUString("com.sun.star.util.URLTransformer"), mContext);
                        if (testInstance.is())
                        {
                            // 连接有效，返回加载器
                            return mLoader;
                        }
                    }
                    catch (const uno::Exception&)
                    {
                        // 忽略测试实例创建的异常，继续执行重连逻辑
                    }
                }
            }
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "UNO Exception during connection check: " 
                      << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() 
                      << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "STD Exception during connection check: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception during connection check" << std::endl;
        }

        // 如果连接无效，尝试重新连接
        std::cout << "LibreOffice connection lost, attempting to reconnect..." << std::endl;
        
        // 释放旧的连接资源
        release(); 
        
        // 尝试重新初始化连接
        if (initialize())
        {
            std::cout << "Successfully reconnected to LibreOffice service" << std::endl;
            return mLoader;
        }

        // 所有尝试都失败了
        std::cerr << "Failed to reconnect to LibreOffice" << std::endl;
        return nullptr;
    }

    // 获取组件上下文
    uno::Reference<uno::XComponentContext> LibreOfficeConnectionManager::getContext()
    {
        // 如果没有连接，先尝试初始化连接
        if (!mIsConnected)
        {
            if (!initialize())
            {
                return nullptr;
            }
        }
        
        // 检查连接是否仍然有效
        try
        {
            if (mContext.is())
            {
                // 尝试执行一个简单的操作来验证连接
                uno::Reference<lang::XMultiComponentFactory> xMCF_base = mContext->getServiceManager();
                if (xMCF_base.is())
                {
                    return mContext;
                }
            }
        }
        catch (const uno::Exception &e)
        {
            std::cerr << "UNO Exception during context connection check: " 
                      << rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() 
                      << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "STD Exception during context connection check: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown exception during context connection check" << std::endl;
        }

        // 如果连接无效，尝试重新连接
        std::cout << "LibreOffice context connection lost, attempting to reconnect..." << std::endl;
        release(); // 释放旧的连接
        if (initialize())
        {
            std::cout << "Successfully reconnected to LibreOffice service" << std::endl;
            return mContext;
        }

        std::cerr << "Failed to reconnect to LibreOffice context" << std::endl;
        return nullptr;
    }

    // 释放连接
    void LibreOfficeConnectionManager::release()
    {
        // 使用 reset() 替代 clear() 以避免段错误
        mLoader = nullptr;
        mContext = nullptr;

        mIsConnected = false;
    }
} // namespace filemanager