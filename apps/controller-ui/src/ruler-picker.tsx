import { useEffect, useMemo, useRef, useState } from "react";

const tickHeight = 124;
const visibleTicks = 7;

type RulerPickerProps = {
  step: number;
  min: number;
  max: number;
  suffix?: string;
  value: number;
  onChange: (next: number) => void;
};

function RulerPicker({ step, min, max, suffix = "", value, onChange }: RulerPickerProps) {
  const items = useMemo(
    () =>
      Array.from(
        { length: Math.floor((max - min) / step) + 1 },
        (_, i) => min + i * step
      ),
    [max, min, step]
  );
  const defaultViewportHeight = tickHeight * visibleTicks;
  const [viewportHeight, setViewportHeight] = useState(defaultViewportHeight);
  const listHeight = items.length * tickHeight;
  const centerY = viewportHeight / 2 - tickHeight / 2;
  const valueToOffset = (val: number) =>
    centerY - ((val - min) / step) * tickHeight;
  const [offsetY, setOffsetY] = useState(() => valueToOffset(value));
  const viewportRef = useRef<HTMLDivElement | null>(null);
  const draggingRef = useRef(false);
  const startYRef = useRef(0);
  const startOffsetRef = useRef(0);
  const animRef = useRef<number | null>(null);
  const velocityRef = useRef(0);
  const lastTimeRef = useRef<number | null>(null);

  const clamp = (val: number) => Math.max(min, Math.min(max, val));
  const clampOffset = (val: number) => {
    const maxOffset = centerY;
    const minOffset = centerY - (listHeight - tickHeight);
    return Math.max(minOffset, Math.min(maxOffset, val));
  };
  const getOffsetLimits = () => {
    const maxOffset = centerY;
    const minOffset = centerY - (listHeight - tickHeight);
    return { minOffset, maxOffset };
  };
  const offsetToValue = (val: number) => {
    const index = Math.round((centerY - val) / tickHeight);
    const clampedIndex = Math.max(0, Math.min(items.length - 1, index));
    return min + clampedIndex * step;
  };
  const rubberBand = (x: number, minVal: number, maxVal: number) => {
    const constant = 0.55;
    if (x < minVal) {
      const overshoot = minVal - x;
      return minVal - overshoot * constant;
    }
    if (x > maxVal) {
      const overshoot = x - maxVal;
      return maxVal + overshoot * constant;
    }
    return x;
  };
  const stopAnimation = () => {
    if (animRef.current != null) {
      cancelAnimationFrame(animRef.current);
      animRef.current = null;
    }
    lastTimeRef.current = null;
    velocityRef.current = 0;
  };
  const startSnapAnimation = (from: number, to: number) => {
    stopAnimation();
    const stiffness = 0.08;
    const damping = 0.78;

    let current = from;
    let velocity = velocityRef.current;
    const stepFrame = (time: number) => {
      if (lastTimeRef.current == null) lastTimeRef.current = time;
      const dt = Math.min(32, time - lastTimeRef.current);
      lastTimeRef.current = time;

      const displacement = to - current;
      const accel = displacement * stiffness;
      velocity = (velocity + accel * (dt / 16)) * damping;
      current += velocity * (dt / 16);

      if (Math.abs(displacement) < 0.5 && Math.abs(velocity) < 0.1) {
        setOffsetY(to);
        onChange(clamp(offsetToValue(to)));
        stopAnimation();
        return;
      }

      setOffsetY(current);
      onChange(clamp(offsetToValue(current)));
      animRef.current = requestAnimationFrame(stepFrame);
    };
    animRef.current = requestAnimationFrame(stepFrame);
  };

  const onPointerDown = (e: React.PointerEvent) => {
    (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    stopAnimation();
    startYRef.current = e.clientY;
    startOffsetRef.current = offsetY;
    draggingRef.current = true;
  };

  const onPointerMove = (e: React.PointerEvent) => {
    if (!draggingRef.current) return;
    const dy = e.clientY - startYRef.current;
    const rawOffset = startOffsetRef.current + dy;
    const { minOffset, maxOffset } = getOffsetLimits();
    const nextOffset = rubberBand(rawOffset, minOffset, maxOffset);
    setOffsetY(nextOffset);
    onChange(clamp(offsetToValue(clampOffset(rawOffset))));
  };

  const onPointerUp = (e: React.PointerEvent) => {
    draggingRef.current = false;
    (e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId);
    const { minOffset, maxOffset } = getOffsetLimits();
    const clamped = clampOffset(offsetY);
    if (offsetY < minOffset || offsetY > maxOffset) {
      startSnapAnimation(offsetY, clamped);
    }
  };

  useEffect(() => {
    if (!viewportRef.current) return;
    const node = viewportRef.current;
    const update = () => {
      const height = node.getBoundingClientRect().height;
      if (height > 0) {
        setViewportHeight(height);
      }
    };
    update();
    const observer = new ResizeObserver(update);
    observer.observe(node);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (draggingRef.current) return;
    setOffsetY(clampOffset(valueToOffset(value)));
  }, [viewportHeight, value]);

  useEffect(() => stopAnimation, []);

  return (
    <div className="text-white w-40" style={{ height: "100%" }}>
      <div
        ref={viewportRef}
        className="bg-black"
        style={{
          position: "relative",
          height: "100%",
          overflow: "hidden"
        }}
      >
        <div
          onPointerDown={onPointerDown}
          onPointerMove={onPointerMove}
          onPointerUp={onPointerUp}
          onPointerCancel={onPointerUp}
          style={{
            background: "black",
            position: "absolute",
            left: 0,
            top: 0,
            transform: `translateY(${offsetY}px)`,
            width: "100%",
            cursor: "ns-resize",
            touchAction: "none",
            display: "flex",
            flexDirection: "column",
          }}
        >
          {items.map((val) => (
            <div
              key={val}
              className="flex items-center select-none"
              style={{
                //background: value === val ? "#0e0e0e" : undefined,
                height: `${tickHeight}px`,
                paddingLeft: "8px",
              }}
            >
              <span className="text-4xl mr-4 whitespace-nowrap">
                {val} {suffix}
              </span>

              <div
                className="w-full h-0.5 bg-zinc-500"
              />
            </div>
          ))}
        </div>

        <div
          style={{
            position: "absolute",
            left: 0,
            right: 0,
            top: `${centerY + tickHeight / 2 - 1}px`,
            height: "2px",
            background: "red",
            pointerEvents: "none",
          }}
        />
        <div
          style={{
            position: "absolute",
            left: 0,
            right: 0,
            top: 0,
            height: `${tickHeight * 1.5}px`,
            background: "linear-gradient(to bottom, rgba(0,0,0,1), rgba(0,0,0,0))",
            pointerEvents: "none",
          }}
        />
        <div
          style={{
            position: "absolute",
            left: 0,
            right: 0,
            bottom: 0,
            height: `${tickHeight * 1.5}px`,
            background: "linear-gradient(to top, rgba(0,0,0,1), rgba(0,0,0,0))",
            pointerEvents: "none",
          }}
        />
      </div>
    </div>
  );
}

export { RulerPicker };
