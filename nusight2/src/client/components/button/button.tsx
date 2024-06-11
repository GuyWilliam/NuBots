import React from "react";
import classNames from "classnames";

import { Icon, IconProps } from "../icon/view";

type ButtonIconProps = Pick<IconProps, "fill" | "weight" | "flip" | "rotate" | "className">;

export type ButtonProps = React.ButtonHTMLAttributes<HTMLButtonElement> & {
  /** The size of the button */
  size?: "normal" | "large";
  /** The color of the button */
  color?: "default" | "primary" | "transparent";
  /** The alignment of the child elements of the button */
  contentAlign?: "left" | "center" | "right";
  /** Icon positioned before the children */
  iconBefore?: React.ReactNode;
  /** Props for the `<Icon>` rendered before the children */
  iconBeforeProps?: ButtonIconProps;
  /** Icon positioned after the children */
  iconAfter?: React.ReactNode;
  /** Props for the `<Icon>` rendered after the children */
  iconAfterProps?: ButtonIconProps;
};

const ColorToClassName = {
  default:
    "shadow-sm bg-white text-gray-600 ring-1 ring-inset ring-gray-300 dark:bg-gray-600 dark:text-white dark:ring-gray-500",
  primary: "dark shadow-sm bg-blue-500 text-white",
  transparent: "bg-transparent text-blue-500 ring-transparent dark:text-blue-500",
} as const;

const SizeToClassName = {
  normal: "h-8 [&_svg]:w-5 [&_svg]:h-5 px-4",
  large: "h-10 [&_svg]:w-5 [&_svg]:h-5 px-6",
} as const;

const ContentAlignToClassName = {
  center: "justify-center",
  left: "justify-start",
  right: "justify-end",
} as const;

/**
 * Convert a node to an Icon if it is a string. Otherwise return the node,
 * assuming it's already an Icon.
 */
function createIcon(node: React.ReactNode, props?: ButtonIconProps): React.ReactNode {
  return typeof node === "string" ? (
    <Icon size="20" {...props}>
      {node}
    </Icon>
  ) : (
    node
  );
}

export function Button(props: ButtonProps) {
  const {
    color = "default",
    size = "normal",
    contentAlign = "center",
    className,
    iconBefore,
    iconBeforeProps,
    iconAfter,
    iconAfterProps,
    children,
    ...buttonProps
  } = props;

  return (
    <button
      {...buttonProps}
      className={classNames(
        "relative gap-2 inline-flex items-center rounded disabled:opacity-40 pt-[1px] group",
        SizeToClassName[size],
        ColorToClassName[color],
        ContentAlignToClassName[contentAlign],
        {
          // Decrease padding on the side(s) containing an icon
          "pl-3": iconBefore,
          "pr-3": iconAfter,
        },
        className,
      )}
    >
      {createIcon(iconBefore, iconBeforeProps)}
      {children}
      {createIcon(iconAfter, iconAfterProps)}
      <span
        className={classNames(
          "absolute inset-0 rounded group-hover:bg-auto-contrast-1 group-active:bg-auto-contrast-2 pointer-events-none",
        )}
      />
    </button>
  );
}
