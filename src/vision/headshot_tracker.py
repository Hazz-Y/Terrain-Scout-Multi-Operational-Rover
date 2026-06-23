"""
Terrain Scout III — HeadShot Tracker Module
============================================

AI-powered face detection and servo-based auto-tracking system.
Uses Haar cascade classifier to detect faces in real-time via a
USB webcam and translates face coordinates to pan/tilt servo angles
for precision targeting.

Hardware Requirements:
    - USB Webcam (1080p recommended)
    - 2x Servo Motors (MG995 — pan & tilt)
    - Arduino UNO/Nano (for servo control via serial)
    - Raspberry Pi 4 (running this script)

Author: Avishkar Jaiswal
Project: Terrain Scout III — Multi-Operational Defence Rover
"""

import cv2
import numpy as np

# ─── Configuration ─────────────────────────────────────────────
FRAME_WIDTH = 1920
FRAME_HEIGHT = 1080
FPS = 24
CASCADE_PATH = "haarcascade_frontalface_default.xml"

# Servo range mapping
SERVO_MIN = 0
SERVO_MAX = 180

# Face detection parameters
SCALE_FACTOR = 1.1
MIN_NEIGHBORS = 5
MIN_FACE_SIZE = (30, 30)


def initialize_camera(width=FRAME_WIDTH, height=FRAME_HEIGHT, fps=FPS):
    """Initialize USB webcam with specified resolution and framerate."""
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    cap.set(cv2.CAP_PROP_FPS, fps)

    if not cap.isOpened():
        raise RuntimeError("Camera couldn't be accessed! Check USB connection.")

    return cap


def load_face_detector(cascade_path=CASCADE_PATH):
    """Load Haar cascade classifier for frontal face detection."""
    face_cascade = cv2.CascadeClassifier(cascade_path)
    if face_cascade.empty():
        raise FileNotFoundError(f"Cascade file not found: {cascade_path}")
    return face_cascade


def map_to_servo(face_center_x, face_center_y, frame_w, frame_h):
    """
    Convert pixel coordinates to servo angles (0–180°).

    Args:
        face_center_x: X pixel coordinate of face center
        face_center_y: Y pixel coordinate of face center
        frame_w: Frame width in pixels
        frame_h: Frame height in pixels

    Returns:
        Tuple of (servo_x, servo_y) angles clipped to [0, 180]
    """
    servo_x = int(np.interp(face_center_x, [0, frame_w], [SERVO_MIN, SERVO_MAX]))
    servo_y = int(np.interp(face_center_y, [0, frame_h], [SERVO_MIN, SERVO_MAX]))
    return np.clip(servo_x, SERVO_MIN, SERVO_MAX), np.clip(servo_y, SERVO_MIN, SERVO_MAX)


def draw_targeting_overlay(img, face, servo_pos, frame_w, frame_h):
    """
    Draw military-style targeting HUD overlay on detected face.

    Args:
        img: Input frame (modified in-place)
        face: Tuple (x, y, w, h) of detected face bounding box
        servo_pos: Current [servo_x, servo_y] positions
        frame_w: Frame width
        frame_h: Frame height
    """
    fx, fy, fw, fh = face
    cx, cy = fx + fw // 2, fy + fh // 2

    # Targeting reticle
    cv2.circle(img, (cx, cy), 80, (0, 0, 255), 2)
    cv2.circle(img, (cx, cy), 15, (0, 0, 255), cv2.FILLED)

    # Crosshair lines
    cv2.line(img, (0, cy), (frame_w, cy), (0, 0, 0), 2)
    cv2.line(img, (cx, frame_h), (cx, 0), (0, 0, 0), 2)

    # Status text
    cv2.putText(img, f"[{fx}, {fy}]", (cx + 15, cy - 15),
                cv2.FONT_HERSHEY_PLAIN, 2, (255, 0, 0), 2)
    cv2.putText(img, "TARGET LOCKED", (frame_w - 350, 50),
                cv2.FONT_HERSHEY_PLAIN, 2, (255, 0, 255), 2)

    # Servo telemetry
    cv2.putText(img, f'Servo X: {int(servo_pos[0])} deg', (50, 50),
                cv2.FONT_HERSHEY_PLAIN, 1.5, (255, 0, 0), 2)
    cv2.putText(img, f'Servo Y: {int(servo_pos[1])} deg', (50, 80),
                cv2.FONT_HERSHEY_PLAIN, 1.5, (255, 0, 0), 2)


def draw_idle_overlay(img, servo_pos, frame_w, frame_h):
    """Draw HUD overlay when no target is detected."""
    cx, cy = frame_w // 2, frame_h // 2

    cv2.putText(img, "NO TARGET", (frame_w - 250, 50),
                cv2.FONT_HERSHEY_PLAIN, 2, (0, 0, 255), 2)
    cv2.circle(img, (cx, cy), 80, (0, 0, 255), 2)
    cv2.circle(img, (cx, cy), 15, (0, 0, 255), cv2.FILLED)
    cv2.line(img, (0, cy), (frame_w, cy), (0, 0, 0), 2)
    cv2.line(img, (cx, frame_h), (cx, 0), (0, 0, 0), 2)

    cv2.putText(img, f'Servo X: {int(servo_pos[0])} deg', (50, 50),
                cv2.FONT_HERSHEY_PLAIN, 1.5, (255, 0, 0), 2)
    cv2.putText(img, f'Servo Y: {int(servo_pos[1])} deg', (50, 80),
                cv2.FONT_HERSHEY_PLAIN, 1.5, (255, 0, 0), 2)


def main():
    """Main tracking loop — detects faces and maps to servo coordinates."""
    cap = initialize_camera()
    face_cascade = load_face_detector()
    servo_pos = [90, 90]  # Initial center position

    print("[TS-III] HeadShot Tracker initialized. Press ESC to exit.")

    while True:
        success, img = cap.read()
        if not success:
            continue

        # Convert to grayscale for face detection
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(
            gray,
            scaleFactor=SCALE_FACTOR,
            minNeighbors=MIN_NEIGHBORS,
            minSize=MIN_FACE_SIZE
        )

        if len(faces) > 0:
            fx, fy, fw, fh = faces[0]  # Track first detected face
            servo_x, servo_y = map_to_servo(
                fx + fw / 2, fy + fh / 2, FRAME_WIDTH, FRAME_HEIGHT
            )
            servo_pos = [servo_x, servo_y]

            # TODO: Send servo commands via serial
            # serial_conn.write(f"{servo_x},{servo_y}\n".encode())

            draw_targeting_overlay(img, faces[0], servo_pos, FRAME_WIDTH, FRAME_HEIGHT)
        else:
            draw_idle_overlay(img, servo_pos, FRAME_WIDTH, FRAME_HEIGHT)

        cv2.imshow("TS-III HeadShot Tracker", img)

        if cv2.waitKey(1) == 27:  # ESC key
            break

    cap.release()
    cv2.destroyAllWindows()
    print("[TS-III] HeadShot Tracker shutdown complete.")


if __name__ == "__main__":
    main()