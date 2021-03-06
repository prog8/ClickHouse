#pragma once

#include <memory>
#include <boost/noncopyable.hpp>

#include <Poco/Util/Application.h>

#include <ext/singleton.h>

#include <mysqlxx/Query.h>
#include <mysqlxx/Exception.h>

#define MYSQLXX_DEFAULT_TIMEOUT 60
#define MYSQLXX_DEFAULT_RW_TIMEOUT 1800


namespace mysqlxx
{


/** LibrarySingleton is used for appropriate initialisation and deinitialisation of MySQL library.
  * Makes single thread-safe call of mysql_library_init().
  * Usage:
  *     LibrarySingleton::instance();
  */
class LibrarySingleton : public ext::singleton<LibrarySingleton>
{
friend class ext::singleton<LibrarySingleton>;
private:
    LibrarySingleton();
    ~LibrarySingleton();
};


/** MySQL connection.
  * Usage:
  *        mysqlxx::Connection connection("Test", "127.0.0.1", "root", "qwerty", 3306);
  *        std::cout << connection.query("SELECT 'Hello, World!'").store().at(0).at(0).getString() << std::endl;
  *
  * Or with Poco library configuration:
  *        mysqlxx::Connection connection("mysql_params");
  *
  * Or using socket:
  *        mysqlxx::Connection connection("Test", "localhost", "root", "qwerty", 0, "/path/to/socket/file.sock");
  *
  * Attention! It's strictly recommended to use connection in thread where it was created.
  * In order to use connection in other thread, you should call MySQL C API function mysql_thread_init() before and
  * mysql_thread_end() after working with it.
  */
class Connection : private boost::noncopyable
{
public:
    /// For delayed initialisation
    Connection();

    /// Creates connection. Either port either socket should be specified.
    /// If server is localhost and socket is not empty, than socket is used. Otherwise, server and port is used.
    Connection(
        const char * db,
        const char * server,
        const char * user = 0,
        const char * password = 0,
        unsigned port = 0,
        const char * socket = "",
        unsigned timeout = MYSQLXX_DEFAULT_TIMEOUT,
        unsigned rw_timeout = MYSQLXX_DEFAULT_RW_TIMEOUT);

    /// Creates connection. Can be used if Poco::Util::Application is using.
    /// All settings will be got from config_name section of configuration.
    Connection(const std::string & config_name);

    virtual ~Connection();

    /// Provides delayed initialization or reconnection with other settings.
    virtual void connect(const char * db,
        const char * server,
        const char * user,
        const char * password,
        unsigned port,
        const char * socket,
        unsigned timeout = MYSQLXX_DEFAULT_TIMEOUT,
        unsigned rw_timeout = MYSQLXX_DEFAULT_RW_TIMEOUT);

    void connect(const std::string & config_name)
    {
        Poco::Util::LayeredConfiguration & cfg = Poco::Util::Application::instance().config();

        std::string db = cfg.getString(config_name + ".db", "");
        std::string server = cfg.getString(config_name + ".host");
        std::string user = cfg.getString(config_name + ".user");
        std::string password = cfg.getString(config_name + ".password");
        unsigned port = cfg.getInt(config_name + ".port", 0);
        std::string socket = cfg.getString(config_name + ".socket", "");

        unsigned timeout =
            cfg.getInt(config_name + ".connect_timeout",
                cfg.getInt("mysql_connect_timeout",
                    MYSQLXX_DEFAULT_TIMEOUT));

        unsigned rw_timeout =
            cfg.getInt(config_name + ".rw_timeout",
                cfg.getInt("mysql_rw_timeout",
                    MYSQLXX_DEFAULT_RW_TIMEOUT));

        connect(db.c_str(), server.c_str(), user.c_str(), password.c_str(), port, socket.c_str(), timeout, rw_timeout);
    }

    /// If MySQL connection was established.
    bool connected() const;

    /// Disconnect from MySQL.
    void disconnect();

    /// Tries to reconnect if connection was lost. Is true if connection is established after call.
    bool ping();

    /// Creates query. It can be set with query string or later.
    Query query(const std::string & str = "");

    /// Get MySQL C API MYSQL object.
    MYSQL * getDriver();

private:
    std::unique_ptr<MYSQL> driver;
    bool is_connected;
};


}
