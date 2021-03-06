//
// Created by warnelso on 6/6/16.
//

#include <iostream>
#include <regex>
#include "DownloadHandler.h"
#include "DownloadHash.h"

DownloadHandler::DownloadHandler(Filename filename) {
    add_download(filename);
    _current_download = nullptr;

}


DownloadHandler::DownloadHandler(std::list<Filename> files) {
    for (auto &filename : files) {
        add_download(filename);
    }
    _current_download = nullptr;
}

DownloadHandler::~DownloadHandler() {
    for (auto &element: _completed_downloads) { // delete all completed downloads
        delete element.second;
    }
    if (currently_downloading()) {
        delete_current_download(); // delete incomplete download
    }
}

bool DownloadHandler::add_download(const Filename &filename) {
    if (downloaded(filename)  // if downloaded
        || in_download_queue(filename) // or in queue
        || currently_downloading(filename))
        return false; // or currently being downloaded
    else {
        _download_queue.push_back(filename);
        std::cout << "File added to download queue: " << filename << "\n";
        return true;
    }
}

void DownloadHandler::add_downloads_from_config() {
    if (!downloaded("pxelinux.cfg")) throw not_found_exception();
    Download *file = _completed_downloads["pxelinux.cfg"];
    std::string filestr((char *) file->data(), file->size());
    std::smatch matches;
    std::regex regex("(kernel [^ \\n\\r]*|initrd=[^ \\n\\r]*)");
    while (std::regex_search(filestr, matches, regex)) {
        add_download(matches.begin()->str().substr(7));
        filestr = matches.suffix().str();
    }
}

void DownloadHandler::rename_current_downlaod(const Filename &new_filename) {
    if (!currently_downloading()) throw not_found_exception();
    _current_download->first = new_filename;
    _current_download->second->filename(new_filename);
}

void DownloadHandler::append_data(const Data &data, uint16_t block_number) {
    if (currently_downloading()) {
        _current_download->second->add_data(data, block_number);
    }
}

void DownloadHandler::set_current_download_sizes(uint32_t total_size) {
    if (currently_downloading()) {
        _current_download->second->total_size(total_size);
    }
}

void DownloadHandler::finalize_current_download() {
    if (!currently_downloading()) throw not_found_exception();
    else if (!downloaded(_current_download->first)) {
        _current_download->second->finalize();
        std::cout << "Finalized File Data: \n" <<
        std::string((const char *) _current_download->second->data(), _current_download->second->size()) << "\n";
        _completed_downloads[_current_download->first] = _current_download->second;
        _current_download->second = nullptr;
        delete _current_download;
        _current_download = nullptr;
    } else throw overwrite_exception();
}

void DownloadHandler::delete_current_download() {
    if (!currently_downloading()) throw not_found_exception();
    else {
        delete _current_download->second;
        _current_download->second = nullptr;
        delete _current_download;
        _current_download = nullptr;
    }
}

void DownloadHandler::clear_queue() {
    _download_queue.clear();
}

const Filename DownloadHandler::start_new_download_file() {
    if (!currently_downloading()) {
        Filename filename = _download_queue.front();
        _download_queue.pop_front();
        _current_download = new std::pair<Filename, Download *>(filename, new DownloadFile(filename));
        return filename;
    } else throw not_found_exception();
}

const Filename DownloadHandler::start_new_download_hash() {
    if (!currently_downloading()) {
        Filename filename = _download_queue.front();
        _download_queue.pop_front();
        _current_download = new std::pair<Filename, Download *>(filename, new DownloadHash(filename));
        return filename;
    } else throw not_found_exception();
}

bool DownloadHandler::downloaded(const Filename &filename) const {
    return (bool) _completed_downloads.count(filename);
}

bool DownloadHandler::in_download_queue(const Filename &filename) const {
    return std::find(_download_queue.begin(), _download_queue.end(), filename) != _download_queue.end();
}

bool DownloadHandler::currently_downloading(const Filename &filename) const {
    return _current_download != nullptr && _current_download->first == filename;
}

bool DownloadHandler::currently_downloading() const {
    return _current_download != nullptr;
}

bool DownloadHandler::complete() const {
    return (!_current_download && _download_queue.empty());
}

bool DownloadHandler::current_download_complete() const {
    if (currently_downloading()) {
        return _current_download->second->complete();
    } else throw not_found_exception();
}
