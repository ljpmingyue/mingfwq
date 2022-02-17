#include "http_session.h"
#include "http_parser.h"

namespace mingfwq{
namespace http{

    HttpSession::HttpSession(Socket::ptr sock, bool owner)
            :SocketStream(sock,owner){

    }

    HttpRequest::ptr HttpSession::recvRequest(){
        HttpRequestParser::ptr parser(new HttpRequestParser);
        uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
        //uint64_t buff_size = 150;
        
        //智能指针只会当他是一个成员只会delete，但其实是个数组
        std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
        
        char* data = buffer.get();
        int offset = 0;
        do{
            int len = read(data + offset, buff_size - offset);
            if(len <= 0){
                close();
                return nullptr;
            }
            //返回解析长度
            size_t nparesr = parser->execute(data, len, offset);

            if(parser->hasError()){
                close();
                return nullptr;
            }
            offset = len - nparesr;
            if(offset == (int)buff_size){
                close();
                return nullptr;
            }
            if(parser->isFinished()){
                break;
            }
        }while (true);

        //用int64_t 不用uint64_t是在减法操作时候不容易溢出
        int64_t length = parser->getContentLength();
        if(length > 0){
            std::string body;
            body.resize(length);

            int len = 0;
            if(length >= offset){
                //body.append(data,offset);
                memcpy(&body[0], data, offset);
                len = offset;
            }else{
                //body.append(data,length);
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            if(length > 0){
                //if(readFixSize(&body[body.size()],length) <= 0){
                if(readFixSize(&body[len],length) <= 0){
                    close();
                    return nullptr;
                }
                parser->getData()->setBody(body);
            }
            
        }
        
        return parser->getData();
    }

    int HttpSession::sendResponse(HttpResponse::ptr hrt){
        std::stringstream ss;
        ss << *hrt;
        std::string data = ss.str();
        return writeFixSize(data.c_str(),data.size());
    }






}


}