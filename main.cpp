
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

#include <chrono>
#include <format>
#include <future>
#include <thread>

#include "WebViewFiles.h"

class StdoutLogger : public juce::Logger {
 public:
  void logMessage(const juce::String& message) override {
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    std::cout << message.toStdString() << std::endl;
  }
};

// Inherited to observe lifecycle and visibility changes
class MyWebview : public juce::WebBrowserComponent {
 public:
  MyWebview(const juce::WebBrowserComponent::Options& options)
      : juce::WebBrowserComponent(options) {
    juce::Logger::writeToLog("Init webview with options");
  }
  MyWebview() : juce::WebBrowserComponent() {
    juce::Logger::writeToLog("Init webview with defaults");
  }
  ~MyWebview() override { juce::Logger::writeToLog("Destroying webview"); }
  void visibilityChanged() override {
    juce::WebBrowserComponent::visibilityChanged();
    juce::Logger::writeToLog(std::format("Webview visibility changed to {}",
                                         isVisible() ? "visible" : "hidden"));
  }
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyWebview)
};

// Couple functions from webview example to load resources from zip
static const char* getMimeForExtension(const juce::String& extension) {
  static const std::unordered_map<juce::String, const char*> mimeMap = {
      {{"htm"}, "text/html"},
      {{"html"}, "text/html"},
      {{"txt"}, "text/plain"},
      {{"jpg"}, "image/jpeg"},
      {{"jpeg"}, "image/jpeg"},
      {{"svg"}, "image/svg+xml"},
      {{"ico"}, "image/vnd.microsoft.icon"},
      {{"json"}, "application/json"},
      {{"png"}, "image/png"},
      {{"css"}, "text/css"},
      {{"map"}, "application/json"},
      {{"js"}, "text/javascript"},
      {{"woff2"}, "font/woff2"}};

  if (const auto it = mimeMap.find(extension.toLowerCase());
      it != mimeMap.end())
    return it->second;

  jassertfalse;
  return "";
}

std::vector<std::byte> streamToVector(juce::InputStream& stream) {
  using namespace juce;
  const auto sizeInBytes = static_cast<size_t>(stream.getTotalLength());
  std::vector<std::byte> result(sizeInBytes);
  stream.setPosition(0);
  [[maybe_unused]] const auto bytesRead =
      stream.read(result.data(), result.size());
  jassert(bytesRead == static_cast<ssize_t>(sizeInBytes));
  return result;
}

std::vector<std::byte> getWebViewFileAsBytes(const juce::String& filepath) {
  juce::MemoryInputStream zipStream{webview_files::webview_files_zip,
                                    webview_files::webview_files_zipSize,
                                    false};
  juce::ZipFile zipFile{zipStream};

  if (auto* zipEntry = zipFile.getEntry(ZIPPED_FILES_PREFIX + filepath)) {
    const std::unique_ptr<juce::InputStream> entryStream{
        zipFile.createStreamForEntry(*zipEntry)};

    if (entryStream == nullptr) {
      jassertfalse;
      return {};
    }

    return streamToVector(*entryStream);
  }

  return {};
}

void evaluationHandler(juce::WebBrowserComponent::EvaluationResult r) {
  if (r.getResult() == nullptr) {
    jassert(r.getError()->type == juce::WebBrowserComponent::EvaluationResult::
                                      Error::Type::unsupportedReturnType);
    DBG(r.getError()->message);
  }
}
// This is code from emitEvent, but it is private
void emitEventPrivate(juce::WebBrowserComponent& webView,
                      const juce::Identifier& eventId,
                      const juce::var& object) {
  const auto objectAsString = juce::JSON::toString(object, true);
  const auto escaped = objectAsString.replace("\\", "\\\\").replace("'", "\\'");
  webView.evaluateJavascript("window.__JUCE__.backend.emitByBackend(" +
                                 eventId.toString().quoted() + ", " +
                                 escaped.quoted('\'') + ");",
                             evaluationHandler);
};

void scheduleEvent(juce::WebBrowserComponent* webView,
                   size_t secondsToWait = 5) {
  // To be sure that I am sending event if webview is no fully visible -
  // emitEventIfBrowserIsVisible
  std::thread([webView, secondsToWait]() {
    std::this_thread::sleep_for(std::chrono::seconds(secondsToWait));
    juce::Logger::writeToLog(
        std::format("Action executed after {} seconds", secondsToWait));
    juce::MessageManager::callAsync([webView]() {
      static const juce::Identifier eventId{"exampleEvent"};
      static const juce::var valueToEmitByPublicInterface{42.0};
      static const juce::var valueToEmitUsingJSEval{67.0};

      webView->emitEventIfBrowserIsVisible(eventId,
                                           valueToEmitByPublicInterface);
      emitEventPrivate(*webView, eventId, valueToEmitUsingJSEval);
    });
  }).detach();
}

class MainWindow : public juce::DocumentWindow {
 public:
  auto getResource(const juce::String& url) const
      -> std::optional<juce::WebBrowserComponent::Resource> {
    juce::Logger::writeToLog(
        std::format("ResourceProvider called with {}", url.toStdString()));

    const auto resourceToRetrieve =
        url == "/" ? "index.html"
                   : url.fromFirstOccurrenceOf("/", false, false);

    const auto resource = getWebViewFileAsBytes(resourceToRetrieve);
    if (!resource.empty()) {
      const auto extension =
          resourceToRetrieve.fromLastOccurrenceOf(".", false, false);
      return juce::WebBrowserComponent::Resource{
          std::move(resource), getMimeForExtension(extension)};
    }

    return std::nullopt;
  }

  MainWindow(juce::String name)
      : DocumentWindow(
            name,
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                juce::ResizableWindow::backgroundColourId),
            allButtons) {
    // Testing native function call from JavaScript
    juce::WebBrowserComponent::NativeFunction sayHelloToJuce =
        [this](const juce::var& params,
               juce::WebBrowserComponent::NativeFunctionCompletion completion) {
          juce::Logger::writeToLog("sayHelloToJuce called from JavaScript");
          juce::Logger::writeToLog("Scheduling event emission in 5 seconds");
          scheduleEvent(webView.get(), 5);
          auto message = params[0].toString();
          completion("Received in C++: " + message);
        };
    juce::File cacheDir = juce::File::getSpecialLocation(
                              juce::File::SpecialLocationType::tempDirectory)
                              .getChildFile("JuceWebView2Cache");
    constexpr auto LOCAL_DEV_SERVER_ADDRESS = "http://app.local";

    auto options =
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withBackgroundColour(juce::Colours::white)
                    .withUserDataFolder(cacheDir))
            .withNativeIntegrationEnabled()
            .withResourceProvider(
                [this](const auto& url) { return getResource(url); },
                juce::URL{LOCAL_DEV_SERVER_ADDRESS}.getOrigin())
            .withInitialisationData("vendor", "COMPANY")
            .withInitialisationData("pluginName", "PRODUCT")
            .withInitialisationData("pluginVersion", "1.0.0")
            .withUserScript(
                "console.log(\"C++ backend here: This is run "
                "before any other loading happens\");")
            .withNativeFunction("sayHelloToJuce", sayHelloToJuce);

    if (cacheDir.exists()) {
      juce::Logger::writeToLog("Cache directory exists");
    } else {
      juce::Logger::writeToLog("Cache directory does not exist");
    }
    if (cacheDir.createDirectory().wasOk()) {
      juce::Logger::writeToLog("Cache directory created successfully");
    } else {
      juce::Logger::writeToLog("Failed to create cache directory");
    }

    webView = std::make_unique<MyWebview>(options);
    addAndMakeVisible(webView.get());
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setUsingNativeTitleBar(true);
    setVisible(true);
    setResizable(true, true);
    setSize(1024, 768);
  #if JUCE_IOS || JUCE_ANDROID
    setFullScreen(true);
  #endif
  }

  void closeButtonPressed() override {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
  }

  void resized() override { webView->setBounds(getLocalBounds()); }

 private:
  std::unique_ptr<juce::WebBrowserComponent> webView;
  juce::TextButton emitJavaScriptEventButton{"Emit JavaScript event"};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class HelloWorldApp : public juce::JUCEApplication {
 public:
  const juce::String getApplicationName() override { return "Hello World"; }
  const juce::String getApplicationVersion() override { return "1.0.0"; }

  void initialise(const juce::String&) override {
    auto* logger = new StdoutLogger();
    juce::Logger::setCurrentLogger(logger);

    // Some flags that might improve aggressive background tab throttling
    // behavior of WebView2
#ifdef _WIN32
    _putenv_s("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS",
              "--disable-backgrounding-occluded-windows "
              "--disable-background-timer-throttling "
              "--disable-features=IsolateOrigins,site-per-process "
              "--disable-features=CalculateNativeWinOcclusion");
#endif
    mainWindow = std::make_unique<MainWindow>(getApplicationName());
  }

  void shutdown() override { mainWindow.reset(); }

 private:
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(HelloWorldApp)