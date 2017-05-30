

#include "InstanceTracker.h"

#include <cassert>

namespace instreclib {
namespace reconstruction {

using namespace std;
using namespace instreclib::segmentation;

void InstanceTracker::ProcessInstanceViews(int frame_idx, const vector<InstanceView> &new_views) {
  // 0. Convert the instance segmentation result (`new_views`) into track frame objects.
  list<TrackFrame> new_track_frames;
  for (const InstanceView &view : new_views) {
    new_track_frames.emplace_back(frame_idx, view);
  }

  // 1. Try to find a matching track for every new frame.
  this->AssignToTracks(new_track_frames);

  // 2. For leftover detections, put them into new, single-frame, tracks.
  cout << new_track_frames.size() << " new unassigned frames." << endl;
  for (const TrackFrame &track_frame : new_track_frames) {
    int track_id = track_count_;
    track_count_++;
    Track new_track(track_id);
    new_track.AddFrame(track_frame);
    this->id_to_active_track_.emplace(make_pair(track_id, new_track));
  }

  // 3. Iterate through existing tracks, find ``expired'' ones, and discard them.
  this->PruneTracks(frame_idx);
}

void InstanceTracker::PruneTracks(int current_frame_idx) {
  auto it = id_to_active_track_.begin();
  while (it != id_to_active_track_.end()) {
    int last_active = it->second.GetEndTime();
    int frame_delta = current_frame_idx - last_active;

    if (frame_delta > inactive_frame_threshold_) {
      it = id_to_active_track_.erase(it);
    } else {
      ++it;
    }
  }
}

std::pair<Track *, float> InstanceTracker::FindBestTrack(const TrackFrame &track_frame) {
  if (id_to_active_track_.empty()) {
    return kNoBestTrack;
  }

  float best_score = -1.0f;
  Track *best_track = nullptr;

  for (auto &entry : id_to_active_track_) {
    const Track& track = entry.second;
    float score = track.ScoreMatch(track_frame);
    if (score > best_score) {
      best_score = score;
      best_track = &entry.second;
    }
  }

  assert(best_score >= 0.0f);
  assert(best_track != nullptr);
  return std::pair<Track *, float>(best_track, best_score);
}

void InstanceTracker::AssignToTracks(std::list<TrackFrame> &new_detections) {
  auto it = new_detections.begin();
  while (it != new_detections.end()) {
    pair<Track *, float> match = FindBestTrack(*it);
    Track *track = match.first;
    float score = match.second;

    if (score > kTrackScoreThreshold) {
      cout << "Found a match based on overlap with score " << score << "." << endl;
      cout << "Adding it to track #" << track->GetId() << " of length " << track->GetSize() << "."
           << endl;

      track->AddFrame(*it);
      it = new_detections.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace segmentation
}  // namespace instreclib
