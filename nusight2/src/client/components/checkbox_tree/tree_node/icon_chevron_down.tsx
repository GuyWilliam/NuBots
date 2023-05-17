import React from "react";

export default function IconChevronDown(props: { className?: string }) {
  return (
    <svg
      className={props.className}
      version="1.1"
      xmlns="http://www.w3.org/2000/svg"
      width="32"
      height="32"
      viewBox="0 0 32 32"
      fill="#757575"
    >
      <path d="M24.933 11.067c-0.533-0.533-1.333-0.533-1.867 0l-7.067 7.067-7.067-7.067c-0.533-0.533-1.333-0.533-1.867 0s-0.533 1.333 0 1.867l8 8c0.267 0.267 0.667 0.4 0.933 0.4s0.667-0.133 0.933-0.4l8-8c0.533-0.533 0.533-1.333 0-1.867z"></path>
    </svg>
  );
}
